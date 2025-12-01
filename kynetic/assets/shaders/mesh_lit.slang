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
    nointerpolation uint meshlet_index;
};

ConstantBuffer<SceneData>                   scene;
[[vk::binding(0, 1)]] Sampler2D             textures[];
[[vk::push_constant]] MeshDrawPushConstants constants;

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(128, 1, 1)]
void mesh_main(out indices uint3 triangles[124], 
               out vertices CoarseVertex vertices[64],
               uint group_id: SV_GroupID,
               uint thread_id: SV_GroupThreadID)
{
    uint32_t*       meshlet_vertex_indices = (uint32_t*)constants.meshlet_vertices;
    uint8_t*        meshlet_triangles      = (uint8_t*)constants.meshlet_triangles;
    float4*         positions              = (float4*)constants.positions;
    Vertex*         vertex_data            = (Vertex*)constants.vertices;
    MeshletData*    meshlets               = (MeshletData*)constants.meshlets;
    InstanceData*   instances              = (InstanceData*)constants.instances;

    uint meshlet_index = group_id;
    MeshletData meshlet = meshlets[meshlet_index];
    
    InstanceData instance = instances[0];
    
    float4x4 mvp = mul(scene.vp, instance.model);
    float3x3 model_inv = float3x3(instance.model_inv);

    SetMeshOutputCounts(meshlet.vertex_count, meshlet.triangle_count);

    if (thread_id < meshlet.vertex_count)
    {
        uint vertex_index = meshlet_vertex_indices[meshlet.vertex_offset + thread_id];
        
        float3 pos = positions[vertex_index].xyz;
        Vertex v = vertex_data[vertex_index];

        vertices[thread_id].position = mul(mvp, float4(pos, 1.0));
        vertices[thread_id].color = v.color.xyz;
        vertices[thread_id].uv = float2(v.uv_x, v.uv_y);
        
        vertices[thread_id].tangent = normalize(mul(model_inv, v.tangent.xyz));
        vertices[thread_id].bitangent = normalize(mul(model_inv, cross(v.normal, v.tangent.xyz) * v.tangent.w));
        vertices[thread_id].normal = normalize(mul(model_inv, v.normal));
        
        vertices[thread_id].world_position = mul(instance.model, float4(pos, 1.0)).xyz;
        vertices[thread_id].material_index = instance.material_index;
        vertices[thread_id].meshlet_index = meshlet_index;
    }

    if (thread_id < meshlet.triangle_count)
    {
        uint triangle_offset = meshlet.triangle_offset + thread_id * 3;
        triangles[thread_id] = uint3(
            meshlet_triangles[triangle_offset + 0],
            meshlet_triangles[triangle_offset + 1],
            meshlet_triangles[triangle_offset + 2]
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

    float3 light_direction = -scene.sun_direction.xyz;
    float3 light_color = scene.sun_color.xyz;

    float n_dot_l = clamp(dot(coarse_vertex.normal, -scene.sun_direction.xyz), 0.5, 1.0);
    output.color = float4(get_pastel_color(coarse_vertex.meshlet_index) * light_color * n_dot_l, 1.0);

    return output;
}
