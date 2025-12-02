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
    uint32_t instance_index;
    uint32_t meshlet_count;
};

struct MeshDrawPushConstants
{
    VkDeviceAddress draws;
    VkDeviceAddress instances;
    VkDeviceAddress materials;
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

    float4 ambient_color;
    float4 sun_direction;
    float4 sun_color;
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
};
