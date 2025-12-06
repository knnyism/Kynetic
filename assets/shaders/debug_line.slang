#include "shader_shared.slang"

struct DebugVertex
{
    float3 position;
    float4 color;
};

struct DebugPushConstants
{
    VkDeviceAddress vertices;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float4 color;
};

ConstantBuffer<SceneData> scene;
[[vk::push_constant]] DebugPushConstants constants;

[shader("vertex")]
VertexOutput vertex_main(uint vertex_id : SV_VertexID)
{
    DebugVertex* vertices = (DebugVertex*)constants.vertices;
    DebugVertex v = vertices[vertex_id];

    VertexOutput output;
    output.position = mul(scene.vp, float4(v.position, 1.0));
    output.color = v.color;

    return output;
}

[shader("fragment")]
float4 fragment_main(VertexOutput input) : SV_Target
{
    return input.color;
}
