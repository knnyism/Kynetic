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

struct MeshPayload
{
    uint meshlet_indices[32];
};

ConstantBuffer<SceneData>                   scene;
[[vk::binding(0, 1)]] Sampler2D             textures[];
[[vk::push_constant]] MeshDrawPushConstants constants;

bool cone_cull(float3 center, float radius, float3 cone_axis, float cone_cutoff, float3 camera_position)
{
    return dot(center - camera_position, cone_axis) >= cone_cutoff * length(center - camera_position) + radius;
}

groupshared MeshPayload payload;
groupshared uint accepted_count;

[shader("amplification")]
[numthreads(32, 1, 1)]
void amplification_main(uint group_id: SV_GroupID, uint thread_id: SV_GroupThreadID)
{
    if (thread_id == 0) accepted_count = 0;
    GroupMemoryBarrierWithGroupSync();

    InstanceData* instances = (InstanceData*)constants.instances;
    MeshletData* meshlets = (MeshletData*)constants.meshlets;

    uint meshlet_index = group_id * 32 + thread_id;

    bool accept = false;

    if (meshlet_index < constants.meshlet_count) 
    {
        MeshletData meshlet = meshlets[meshlet_index];
        InstanceData instance = instances[0];

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

    if (accept) {
        uint index;
        InterlockedAdd(accepted_count, 1, index);
        payload.meshlet_indices[index] = meshlet_index;
    }
    GroupMemoryBarrierWithGroupSync();

    if (thread_id == 0 && accepted_count > 0) DispatchMesh(accepted_count, 1, 1, payload);
} 

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(124, 1, 1)]
void mesh_main(OutputIndices<uint3, 124> triangles, 
               OutputVertices<CoarseVertex, 64> vertices,
               in payload MeshPayload mesh_payload,
               uint group_id: SV_GroupID, uint thread_id: SV_GroupThreadID)
{
    uint32_t*       meshlet_vertex_indices = (uint32_t*)constants.meshlet_vertices;
    uint8_t*        meshlet_triangles      = (uint8_t*)constants.meshlet_triangles;
    float4*         positions              = (float4*)constants.positions;
    Vertex*         vertex_data            = (Vertex*)constants.vertices;
    InstanceData*   instances              = (InstanceData*)constants.instances;
    MeshletData*    meshlets               = (MeshletData*)constants.meshlets;
    
    uint meshlet_index = mesh_payload.meshlet_indices[group_id];
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
