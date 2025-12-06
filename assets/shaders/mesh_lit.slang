#include "shader_shared.slang"

struct CoarseVertex
{
    float4               position : SV_Position;
    float3               color;
    float2               uv;

    float3               tangent;
    float3               bitangent;
    float3               normal;
                         
    float3               world_position;
    nointerpolation uint material_index;
};

struct MeshPayload
{
    uint meshlet_indices[32];
    uint draw_index;
};

ConstantBuffer<SceneData>                   scene;
[[vk::binding(0, 1)]] Sampler2D             textures[];
[[vk::push_constant]] MeshDrawPushConstants constants;

groupshared MeshPayload payload;

[shader("amplification")]
[numthreads(32, 1, 1)]
void amplification_main(uint group_id: SV_GroupID, uint group_thread_id: SV_GroupThreadID, uint draw_id: SV_DrawIndex)
{
    MeshDrawData* draws = (MeshDrawData*)constants.draws;
    MeshDrawData draw = draws[draw_id];

    InstanceData* instances = (InstanceData*)constants.instances;
    MeshletData* meshlets = (MeshletData*)draw.meshlets;

    uint meshlet_index = group_id * 32 + group_thread_id;

    bool accept = false;

    if (meshlet_index < draw.meshlet_count)
    {
        MeshletData meshlet = meshlets[meshlet_index];
        InstanceData instance = instances[draw.instance_index];

        float4 center = mul(instance.model, float4(meshlet.center, 1.0));
        float scale_x = length(float3(instance.model[0][0], instance.model[1][0], instance.model[2][0]));
        float scale_y = length(float3(instance.model[0][1], instance.model[1][1], instance.model[2][1]));
        float scale_z = length(float3(instance.model[0][2], instance.model[1][2], instance.model[2][2]));
        float scale = max(scale_x, max(scale_y, scale_z));
        float radius = meshlet.radius * scale;

        float3 cone_axis = normalize(mul(float3x3(instance.model), float3(int(meshlet.cone_axis[0]) / 127.0,
                                                                          int(meshlet.cone_axis[1]) / 127.0,
                                                                          int(meshlet.cone_axis[2]) / 127.0)));
        float cone_cutoff = int(meshlet.cone_cutoff) / 127.0;

        accept = !cone_cull(center.xyz, radius, cone_axis, cone_cutoff, scene.view_inv[3].xyz);
        accept = accept && is_visible(scene.vp, center.xyz, radius);
    }

    uint index = WavePrefixCountBits(accept);
    uint count = WaveActiveCountBits(accept);

    if (accept) payload.meshlet_indices[index] = meshlet_index;

    payload.draw_index = draw_id;

    if (group_thread_id == 0 && count > 0) DispatchMesh(count, 1, 1, payload);
}

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(128, 1, 1)]
void mesh_main(OutputIndices<uint3, 128> triangles, 
               OutputVertices<CoarseVertex, 64> vertices,
               in payload MeshPayload mesh_payload,
               uint group_id: SV_GroupID, uint group_thread_id: SV_GroupThreadID)
{
    MeshDrawData* draws = (MeshDrawData*)constants.draws;
    MeshDrawData draw = draws[mesh_payload.draw_index];

    uint32_t*       meshlet_vertex_indices = (uint32_t*)draw.meshlet_vertices;
    uint8_t*        meshlet_triangles_data = (uint8_t*)draw.meshlet_triangles;
    float4*         positions              = (float4*)draw.positions;
    Vertex*         vertex_data            = (Vertex*)draw.vertices;
    InstanceData*   instances              = (InstanceData*)constants.instances;
    MeshletData*    meshlets               = (MeshletData*)draw.meshlets;

    uint meshlet_index = mesh_payload.meshlet_indices[group_id];

    MeshletData meshlet = meshlets[meshlet_index];
    InstanceData instance = instances[draw.instance_index];
    
    float4x4 mvp = mul(scene.vp, instance.model);
    float3x3 model_inv = float3x3(instance.model_inv);

    SetMeshOutputCounts(meshlet.vertex_count, meshlet.triangle_count);

    if (group_thread_id < meshlet.vertex_count)
    {
        uint vertex_index = meshlet_vertex_indices[meshlet.vertex_offset + group_thread_id];
        
        float3 pos = positions[vertex_index].xyz;
        Vertex v = vertex_data[vertex_index];

        vertices[group_thread_id].position = mul(mvp, float4(pos, 1.0));
        vertices[group_thread_id].color = get_pastel_color(meshlet_index);
        vertices[group_thread_id].uv = float2(v.uv_x, v.uv_y);

        vertices[group_thread_id].tangent = normalize(mul(model_inv, v.tangent.xyz));
        vertices[group_thread_id].bitangent = normalize(mul(model_inv, cross(v.normal, v.tangent.xyz) * v.tangent.w));
        vertices[group_thread_id].normal = normalize(mul(model_inv, v.normal));

        vertices[group_thread_id].world_position = mul(instance.model, float4(pos, 1.0)).xyz;
        vertices[group_thread_id].material_index = instance.material_index;
    }

    if (group_thread_id < meshlet.triangle_count)
    {
        uint triangle_offset = meshlet.triangle_offset + group_thread_id * 3;
        triangles[group_thread_id] = uint3(
            meshlet_triangles_data[triangle_offset + 0],
            meshlet_triangles_data[triangle_offset + 1],
            meshlet_triangles_data[triangle_offset + 2]
        );
    }
}

struct Fragment
{
    float4 color;
};

float3 evaluate_light(float3 albedo, float3 V, float3 N, float NoV, float NoL, float roughness, float metallic, float3 f0, float3 L, float3 radiance)
{
    float3 H = normalize(V + L);

    float NoH = saturate(dot(N, H));
    float LoH = saturate(dot(L, H));

    float3 F = schlick(f0, LoH);
    float D = ggx(roughness, NoH);
    float G = smith_ggx_correlated(NoV, NoL, roughness);

    float3 Fd = albedo * (1.0 - F) * (1.0 - metallic) * lambertian();
    float3 Fr = F * D * G;

    return (Fd + Fr) * radiance * NoL;
}

[shader("fragment")]
Fragment fragment_main(CoarseVertex coarse_vertex) : SV_Target
{
    Fragment output;

    MaterialData* materials = (MaterialData*)constants.materials;

    MaterialData material = materials[coarse_vertex.material_index];

    float3 albedo = textures[material.albedo].Sample(coarse_vertex.uv).xyz;
    float3 normal_map = textures[material.normal].Sample(coarse_vertex.uv).xyz * 2.0f - 1.0f;
    float3 metal_rough = textures[material.metal_rough].Sample(coarse_vertex.uv).xyz;
    float3 emissive = textures[material.emissive].Sample(coarse_vertex.uv).xyz;

    float3x3 tbn = float3x3(coarse_vertex.tangent, coarse_vertex.bitangent, coarse_vertex.normal);

    float3 shading_normal = normalize(mul(normal_map, tbn));
    float3 shading_view = normalize(scene.view_inv[3].xyz - coarse_vertex.world_position);

    float linear_roughness = max(metal_rough.g, EPSILON);
    float roughness = linear_roughness * linear_roughness;
    float metallic = metal_rough.b;

    float n_dot_v = max(abs(dot(shading_normal, shading_view)), EPSILON);
    float3 f0 = lerp(0.04, albedo, metallic);

    float3 Lo = 0.0;

    float3 light_direction = -scene.sun_direction.xyz;
    float3 light_color = scene.sun_color.xyz;

    float n_dot_l = saturate(dot(shading_normal, -scene.sun_direction.xyz));
    Lo += evaluate_light(albedo, shading_view, shading_normal, n_dot_v, n_dot_l,
                                                                        roughness,
                                                                        metallic,
                                                                        f0,
                                                                        light_direction,
                                                                        light_color);

    Lo += emissive;

    RenderChannel channel = RenderChannel::Final;
    switch (channel)
    {
        case RenderChannel::Final:              output.color = float4(Lo, 1.0); break;
        case RenderChannel::TexCoords:          output.color = float4(coarse_vertex.uv.x, coarse_vertex.uv.y, 0.0, 1.0); break;
        case RenderChannel::NormalTexture:      output.color = float4(normal_map * 0.5 + 0.5, 1.0); break;
        case RenderChannel::GeometryTangent:    output.color = float4(tbn[0] * 0.5 + 0.5, 1.0); break;
        case RenderChannel::GeometryBitangent:  output.color = float4(tbn[1] * 0.5 + 0.5, 1.0); break;
        case RenderChannel::GeometryNormal:     output.color = float4(tbn[2] * 0.5 + 0.5, 1.0); break;
        case RenderChannel::ShadingNormal:      output.color = float4(shading_normal * 0.5 + 0.5, 1.0); break;
        case RenderChannel::BaseColor:          output.color = float4(albedo, 1.0); break;
        case RenderChannel::Metallic:           output.color = float4(metallic, metallic, metallic, 1.0); break;
        case RenderChannel::Roughness:          output.color = float4(linear_roughness, linear_roughness, linear_roughness, 1.0); break;
        case RenderChannel::Emissive:           output.color = float4(emissive, 1.0); break;
        // case RenderChannel::Occlusion:		ouput.color = float4(occlusion, occlusion, occlusion, 1.0); break;
        default: output.color = float4(1.0, 0.0, 1.0, 1.0); break;
    }

    return output;
}
