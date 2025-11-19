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

struct Vertex
{
    float3 position;
    float uv_x;
    float3 normal;
    float uv_y;
    float4 color;
};

struct DrawPushConstants
{
    column_major float4x4 model;
    VkDeviceAddress vertex_buffer;
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
    column_major float4x4 proj;
    column_major float4x4 vp;
    float4 ambient_color;
    float4 sun_direction;
    float4 sun_color;
};
