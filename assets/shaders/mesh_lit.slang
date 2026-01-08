#include "shader_shared.slang"

struct CoarseVertex
{
    float4               position : SV_Position;
    nointerpolation float3 color;
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
Sampler2D<float>                           depth_pyramid_texture;

[[vk::binding(0, 1)]] Sampler2D             textures[];
[[vk::push_constant]] MeshDrawPushConstants constants;

groupshared MeshPayload payload;

// From clusterlod.h
// camera_proj is projection[1][1], or cot(fovy/2); camera_znear is *positive* near plane distance
// (0..1; multiply by screen height to get pixels) approximate (perspective) projection error of a cluster in screen space
// bounds.error / max(distance(bounds.center, camera_position) - bounds.radius, camera_znear) * (camera_proj * 0.5f)
float get_error_in_screen_space(float3 world_center, float world_radius, float world_error, float3 camera_position)
{   
    float dist = distance(world_center, camera_position) - world_radius;
    dist = max(dist, scene.z_near);

    return constants.screen_height * world_error / (dist * 2.0f * tan(constants.camera_fov_y * 0.5f));

    // this did not work for some reason lol
    // return constants.screen_height * world_error / dist * (scene.projection_11 * 0.5f);
}

bool is_lod_selected(MeshletData meshlet, LODGroupData *lod_groups, float4x4 transform,  float3 camera_position, float scale)
{
    if (constants.force_lod > 0) return meshlet.lod_level == (constants.force_lod - 1);

    // From clusterlod.h
    // cluster should be rendered if:
    // 1. clodGroup::simplified for the group it's in is over error threshold
    // 2. cluster.refined is -1 *or* clodGroup::simplified for groups[cluster.refined].simplified is at or under error threshold

    LODGroupData cluster_group = lod_groups[meshlet.group_id];

    float4 cluster_center = mul(transform, float4(cluster_group.center, 1.0));
    float cluster_error = get_error_in_screen_space(cluster_center.xyz, cluster_group.radius * scale, cluster_group.error * scale, camera_position);

    if (meshlet.parent_group_id == -1) return cluster_error > constants.lod_error_threshold;

    LODGroupData refined_group = lod_groups[meshlet.parent_group_id];

    float4 refined_center = mul(transform, float4(refined_group.center, 1.0));
    float refined_error = get_error_in_screen_space(refined_center.xyz, refined_group.radius * scale, refined_group.error * scale, camera_position);

    return cluster_error > constants.lod_error_threshold && refined_error <= constants.lod_error_threshold;
}

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

        LODGroupData* lod_groups = (LODGroupData*)draw.lod_groups;

        accept = is_lod_selected(meshlet, lod_groups, instance.model, cull_camera_pos, scale);

        if (accept) {
            float3 cone_axis = normalize(mul(float3x3(instance.model), float3(
                int(meshlet.cone_axis[0]) / 127.0,
                int(meshlet.cone_axis[1]) / 127.0,
                int(meshlet.cone_axis[2]) / 127.0)));
            float cone_cutoff = int(meshlet.cone_cutoff) / 127.0;

            accept = !cone_cull(center.xyz, radius, cone_axis, cone_cutoff, cull_camera_pos);
            accept = accept && is_visible(cull_vp, center.xyz, radius);

            float4 aabb;
            if (accept)
            {
                float4x4 cull_view = scene.use_debug_culling > 0 ? scene.debug_view : scene.view;
                float4 view_center = mul(cull_view, center);

                if (project_sphere(view_center.xyz, radius, scene.z_near, scene.projection_00, scene.projection_11, aabb))
                {
                    float width;
                    float height;
                    depth_pyramid_texture.GetDimensions(width, height);

                    float aabb_width = (aabb.z - aabb.x) * width;
                    float aabb_height = (aabb.w - aabb.y) * height;

                    float level = floor(log2(max(aabb_width, aabb_height)));

                    float2 uv = (aabb.xy + aabb.zw) * 0.5;

                    float depth = depth_pyramid_texture.SampleLevel(uv, level);
                    depth = min(depth, depth_pyramid_texture.SampleLevel(float2(aabb.x, 1.0f - aabb.y), level));
                    depth = min(depth, depth_pyramid_texture.SampleLevel(float2(aabb.z, 1.0f - aabb.w), level));
                    depth = min(depth, depth_pyramid_texture.SampleLevel(float2(aabb.x, 1.0f - aabb.w), level));
                    depth = min(depth, depth_pyramid_texture.SampleLevel(float2(aabb.z, 1.0f - aabb.y), level));

                    float3 dir = normalize(cull_camera_pos - center.xyz);
                    float4 sphere_clip = mul(scene.previous_vp, float4(center.xyz + dir * radius, 1.0));
                    float depth_sphere = sphere_clip.z / sphere_clip.w;

                    accept = (depth_sphere >= depth);
                }
            }
        }
    }

    uint index = WavePrefixCountBits(accept);
    uint count = WaveActiveCountBits(accept);

    if (accept) payload.meshlet_indices[index] = meshlet_index;

    payload.draw_index = draw_id;

    if (group_thread_id == 0 && count > 0)
        DispatchMesh(count, 1, 1, payload);
}

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(64, 1, 1)]
void mesh_main(
    OutputIndices<uint3, 64> triangles, 
    OutputVertices<CoarseVertex, 64> vertices,
    in payload MeshPayload mesh_payload,
    uint group_id: SV_GroupID,
    uint group_thread_id: SV_GroupThreadID)
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

        switch (constants.meshlet_render_mode)
        {
        case DebugMeshletRenderMode::Lods:
            vertices[group_thread_id].color = get_lod_color(meshlet.lod_level);
            break;
        case DebugMeshletRenderMode::Groups:
            vertices[group_thread_id].color = get_pastel_color(meshlet.group_id);
            break;
        case DebugMeshletRenderMode::Cluster:
            vertices[group_thread_id].color = get_pastel_color(meshlet_index);
            break;
        case DebugMeshletRenderMode::Triangle:
            vertices[group_thread_id].color = get_pastel_color(group_thread_id);
            break;
        case DebugMeshletRenderMode::Solid:
            vertices[group_thread_id].color = get_pastel_color(draw.instance_index);
            break;
        }

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
Fragment fragment_main(CoarseVertex coarse_vertex, float4 position : SV_Position) : SV_Target
{
    float n_dot_l = saturate(dot(coarse_vertex.normal, -scene.sun_direction.xyz));

    Fragment output;
    output.color = float4(coarse_vertex.color * n_dot_l * scene.sun_color.xyz, 1.0);
    return output;
}
