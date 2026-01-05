//
// Created by kenny on 11/13/25.
//

#pragma once

#if defined(__SLANG__)
#define VkDeviceAddress uint64_t
#else
using float3 = glm::vec3;
using float4 = glm::vec4;
using float4x4 = glm::mat4;

using int2 = glm::ivec2;
#define column_major
#endif

enum class RenderChannel
{
    Final,
    TexCoords,
    NormalTexture,
    GeometryNormal,
    GeometryTangent,
    GeometryBitangent,
    ShadingNormal,
    Emissive,
    BaseColor,
    Metallic,
    Roughness,
    Occlusion
};

struct Vertex
{
    float3 normal;
    float uv_x;
    float4 color;
    float4 tangent;
    float uv_y;
};

struct MeshDrawData
{
    VkDeviceAddress positions;
    VkDeviceAddress vertices;
    VkDeviceAddress meshlets;
    VkDeviceAddress meshlet_vertices;
    VkDeviceAddress meshlet_triangles;
    VkDeviceAddress lod_groups;
    uint32_t instance_index;
    uint32_t meshlet_count;
    uint32_t lod_group_count;
    uint32_t pad0;
};

struct MeshDrawPushConstants
{
    VkDeviceAddress draws;
    VkDeviceAddress instances;
    VkDeviceAddress materials;
    float lod_error_threshold;  // Screen-space error threshold in pixels
    float camera_fov_y;
    float screen_height;
    uint32_t force_lod;  // 0 = auto, 1+ = force specific LOD level
};

struct DrawPushConstants
{
    VkDeviceAddress positions;
    VkDeviceAddress vertices;
    VkDeviceAddress materials;

    VkDeviceAddress instances;
};

struct FrustumCullPushConstants
{
    VkDeviceAddress draw_commands;
    uint32_t draw_count;
};

struct SceneData
{
    column_major float4x4 view;
    column_major float4x4 view_inv;
    column_major float4x4 proj;
    column_major float4x4 vp;

    column_major float4x4 previous_vp;

    column_major float4x4 debug_view;
    column_major float4x4 debug_view_inv;
    column_major float4x4 debug_proj;
    column_major float4x4 debug_vp;

    float4 ambient_color;
    float4 sun_direction;
    float4 sun_color;

    uint32_t use_debug_culling;

    float z_near, projection_00, projection_11;
};

struct InstanceData
{
    column_major float4x4 model;
    column_major float4x4 model_inv;
    float4 position;
    uint32_t material_index;
    uint32_t pad, pad2, pad3;
};

struct MaterialData
{
    uint32_t albedo;
    uint32_t normal;
    uint32_t metal_rough;
    uint32_t emissive;
};

struct MeshletData
{
    float3 center;
    float radius;

    int8_t cone_axis[3];
    int8_t cone_cutoff;

    uint32_t vertex_offset;
    uint32_t triangle_offset;
    uint8_t vertex_count;
    uint8_t triangle_count;
    uint8_t lod_level;
    uint8_t pad0;

    int32_t group_id;
    int32_t parent_group_id;
    float lod_error;
    float parent_error;
};

struct LODGroupData
{
    float3 center;
    float radius;
    float error;
    float max_child_error;
    int32_t depth;
    uint32_t cluster_start;
    uint32_t cluster_count;
    uint32_t pad0;
};

struct DebugLineVertex
{
    float3 position;
    float pad0;
    float4 color;
};

struct DebugPushConstants
{
    VkDeviceAddress vertices;
    uint32_t vertex_count;
    uint32_t is_line;
};

struct MeshletDebugData
{
    float3 center;
    float radius;
    float3 cone_axis;
    float cone_cutoff;
    uint32_t is_visible;
    uint32_t lod_level;
    uint32_t pad0, pad1;
};

struct DebugMeshletPushConstants
{
    VkDeviceAddress meshlet_debug_data;
    uint32_t meshlet_count;
    uint32_t show_spheres;
    uint32_t show_cones;
};

struct MeshletStatsData
{
    uint32_t total_meshlets;
    uint32_t visible_meshlets;
    uint32_t pad0, pad1;
};