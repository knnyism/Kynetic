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
    float3 world_position;
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
    output.world_position = v.position;
    output.color = v.color;

    return output;
}

[shader("fragment")]
float4 fragment_main(VertexOutput input) : SV_Target
{
    // Compute face normal from screen-space derivatives for simple shading
    float3 dx = ddx(input.world_position);
    float3 dy = ddy(input.world_position);
    float3 normal = normalize(cross(dx, dy));

    // View direction
    float3 view_dir = normalize(scene.view_inv[3].xyz - input.world_position);

    // Simple lighting for depth perception
    float ndotv = abs(dot(normal, view_dir));
    float lighting = 0.4 + 0.6 * ndotv;

    float4 color = input.color;
    color.rgb *= lighting;

    return color;
}
