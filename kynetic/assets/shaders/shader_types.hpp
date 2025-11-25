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
    float3 position;
    float uv_x;
    float3 normal;
    float uv_y;
    float4 color;
    float4 tangent;
};

struct DrawPushConstants
{
    VkDeviceAddress vertices;
    VkDeviceAddress instances;
    VkDeviceAddress materials;
};

struct GradientPushConstants
{
    float4 data1;
    float4 data2;
    float4 data3;
    float4 data4;

    int2 size;
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

    RenderChannel debug_channel;
};

struct InstanceData
{
    column_major float4x4 model;
    column_major float4x4 model_inv;

    uint32_t material_index;
};

struct MaterialData
{
    uint32_t albedo;
    uint32_t normal;
    uint32_t metal_rough;
    uint32_t emissive;
};