#include "shader_shared.slang"

struct DebugMeshletVertex
{
    float4 position : SV_Position;
    float4 color;
    float3 world_normal;
};

ConstantBuffer<SceneData> scene;

[[vk::push_constant]] DebugMeshletPushConstants constants;

static const uint SPHERE_SEGMENTS = 16;
static const uint SPHERE_RINGS = 12;
static const uint VERTICES_PER_SPHERE = SPHERE_SEGMENTS * SPHERE_RINGS * 6;

static const uint CONE_SEGMENTS = 16;
static const uint VERTICES_PER_CONE = CONE_SEGMENTS * 3 + CONE_SEGMENTS * 3;

float3 get_sphere_vertex(uint local_id, float3 center, float radius)
{
    uint triangle_id = local_id / 6;
    uint vertex_in_tri = local_id % 6;
    
    uint ring = triangle_id / SPHERE_SEGMENTS;
    uint seg = triangle_id % SPHERE_SEGMENTS;
    
    uint2 offsets[6] = {
        uint2(0, 0), uint2(1, 0), uint2(1, 1),
        uint2(0, 0), uint2(1, 1), uint2(0, 1)
    };
    
    uint r = ring + offsets[vertex_in_tri].y;
    uint s = (seg + offsets[vertex_in_tri].x) % SPHERE_SEGMENTS;
    
    float phi = PI * float(r) / float(SPHERE_RINGS);
    float theta = 2.0 * PI * float(s) / float(SPHERE_SEGMENTS);
    
    float3 pos;
    pos.x = sin(phi) * cos(theta);
    pos.y = cos(phi);
    pos.z = sin(phi) * sin(theta);
    
    return center + pos * radius;
}

float3 get_cone_vertex(uint local_id, float3 apex, float3 axis, float radius, float height)
{
    float3 up = abs(axis.y) < 0.99 ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 right = normalize(cross(up, axis));
    float3 forward = cross(axis, right);
    
    float3 base_center = apex + axis * height;
    
    if (local_id < CONE_SEGMENTS * 3)
    {
        uint tri_id = local_id / 3;
        uint vert_in_tri = local_id % 3;
        
        float theta0 = 2.0 * PI * float(tri_id) / float(CONE_SEGMENTS);
        float theta1 = 2.0 * PI * float(tri_id + 1) / float(CONE_SEGMENTS);
        
        if (vert_in_tri == 0)
            return apex;
        else if (vert_in_tri == 1)
            return base_center + (right * cos(theta0) + forward * sin(theta0)) * radius;
        else
            return base_center + (right * cos(theta1) + forward * sin(theta1)) * radius;
    }
    else
    {
        uint base_local = local_id - CONE_SEGMENTS * 3;
        uint tri_id = base_local / 3;
        uint vert_in_tri = base_local % 3;
        
        float theta0 = 2.0 * PI * float(tri_id) / float(CONE_SEGMENTS);
        float theta1 = 2.0 * PI * float(tri_id + 1) / float(CONE_SEGMENTS);
        
        if (vert_in_tri == 0)
            return base_center;
        else if (vert_in_tri == 1)
            return base_center + (right * cos(theta1) + forward * sin(theta1)) * radius;
        else
            return base_center + (right * cos(theta0) + forward * sin(theta0)) * radius;
    }
}

[shader("vertex")]
DebugMeshletVertex vertex_main(uint vertex_id : SV_VertexID)
{
    DebugMeshletVertex output;
    
    MeshletDebugData* meshlet_data = (MeshletDebugData*)constants.meshlet_debug_data;
    
    uint total_sphere_verts = constants.meshlet_count * VERTICES_PER_SPHERE;
    
    float3 position;
    float4 color;
    float3 normal = float3(0, 1, 0);
    
    bool do_show_spheres = constants.show_spheres != 0;
    bool do_show_cones = constants.show_cones != 0;
    
    if (do_show_spheres && vertex_id < total_sphere_verts)
    {
        uint meshlet_id = vertex_id / VERTICES_PER_SPHERE;
        uint local_id = vertex_id % VERTICES_PER_SPHERE;
        
        MeshletDebugData data = meshlet_data[meshlet_id];
        position = get_sphere_vertex(local_id, data.center, data.radius);
        
        if (data.is_visible > 0)
            color = float4(0.0, 1.0, 0.0, 0.3);
        else
            color = float4(1.0, 0.0, 0.0, 0);
            
        normal = normalize(position - data.center);
    }
    else if (do_show_cones)
    {
        uint cone_vertex_id = vertex_id - (do_show_spheres ? total_sphere_verts : 0);
        uint meshlet_id = cone_vertex_id / VERTICES_PER_CONE;
        uint local_id = cone_vertex_id % VERTICES_PER_CONE;
        
        if (meshlet_id < constants.meshlet_count)
        {
            MeshletDebugData data = meshlet_data[meshlet_id];
            
            float cone_height = data.radius * 0.5;
            float cone_radius = data.radius * 0.3 * (1.0 - data.cone_cutoff);

            position = get_cone_vertex(local_id, data.center + data.cone_axis * cone_height, -data.cone_axis, cone_radius, cone_height);
            
            if (data.is_visible > 0)
                color = float4(0.0, 0.5, 1.0, 0.4); 
            else
                color = float4(1.0, 0.5, 0.0, 0.4);
        }
        else
        {
            position = float3(0, 0, 0);
            color = float4(0, 0, 0, 0);
        }
    }
    else
    {
        position = float3(0, 0, 0);
        color = float4(0, 0, 0, 0);
    }
    
    output.position = mul(scene.vp, float4(position, 1.0));
    output.color = color;
    output.world_normal = normal;
    
    return output;
}

[shader("fragment")]
float4 fragment_main(DebugMeshletVertex input) : SV_Target
{
    return input.color;
}
