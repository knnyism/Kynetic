//
// Created by kennypc on 11/13/25.
//

#pragma once

#if defined(__SLANG__)
#define VkDeviceAddress uint64_t
#else
using float3 = glm::vec3;
using float4 = glm::vec4;
using float4x4 = glm::mat4;

using int2 = glm::ivec2;
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
    float4x4 world_matrix;
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
