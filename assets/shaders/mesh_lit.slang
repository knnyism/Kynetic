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

    float4x4 cull_vp = scene.use_debug_culling > 0 ? scene.debug_vp : scene.vp;
    float3 cull_camera_pos = scene.use_debug_culling > 0 ? scene.debug_view_inv[3].xyz : scene.view_inv[3].xyz;

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

        accept = !cone_cull(center.xyz, radius, cone_axis, cone_cutoff, cull_camera_pos);
        accept = accept && is_visible(cull_vp, center.xyz, radius);
    }

    uint index = WavePrefixCountBits(accept);
    uint count = WaveActiveCountBits(accept);
    
    uint meshlets_in_wave = WaveActiveCountBits(meshlet_index < draw.meshlet_count);

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

[shader("fragment")]
Fragment fragment_main(CoarseVertex coarse_vertex) : SV_Target
{
    MaterialData* materials = (MaterialData*)constants.materials;

    MaterialData material = materials[coarse_vertex.material_index];

    float n_dot_l = saturate(dot(coarse_vertex.normal, -scene.sun_direction.xyz));

    Fragment output;
    output.color = float4(coarse_vertex.color * n_dot_l * scene.sun_color.xyz, 1.0);
    return output;
}
