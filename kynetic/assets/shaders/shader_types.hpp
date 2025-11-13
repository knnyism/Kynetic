//
// Created by kennypc on 11/13/25.
//

#pragma once

#if !defined(__SLANG__)
using float3 = glm::vec3;
using float4 = glm::vec4;
using float4x4 = glm::mat4;
#else
#define VkDeviceAddress uint64_t
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
    float4x4 worldMatrix;
    VkDeviceAddress vertexBuffer;
};