#include "shader_shared.slang"

struct DebugVertex
{
    float4 position : SV_Position;
    float4 color;
};

ConstantBuffer<SceneData> scene;

[[vk::push_constant]] DebugPushConstants constants;

[shader("vertex")]
DebugVertex vertex_main(uint vertex_id : SV_VertexID)
{
    DebugVertex output;
    
    DebugLineVertex* vertices = (DebugLineVertex*)constants.vertices;
    
    if (vertex_id >= constants.vertex_count)
    {
        output.position = float4(0, 0, 0, 1);
        output.color = float4(0, 0, 0, 0);
        return output;
    }
    
    DebugLineVertex v = vertices[vertex_id];
    
    output.position = mul(scene.vp, float4(v.position, 1.0));
    output.color = v.color;
    
    return output;
}

[shader("fragment")]
float4 fragment_main(DebugVertex input) : SV_Target
{
    return input.color;
}