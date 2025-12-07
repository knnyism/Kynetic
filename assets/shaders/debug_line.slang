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

    uint line_index = vertex_id / 6;
    uint quad_vertex = vertex_id % 6;
    
    uint v0_idx = line_index * 2;
    uint v1_idx = line_index * 2 + 1;
    
    if (v1_idx >= constants.vertex_count)
    {
        output.position = float4(0, 0, 0, 1);
        output.color = float4(0, 0, 0, 0);
        return output;
    }
    
    DebugLineVertex v0 = vertices[v0_idx];
    DebugLineVertex v1 = vertices[v1_idx];
    
    float4 p0_clip = mul(scene.vp, float4(v0.position, 1.0));
    float4 p1_clip = mul(scene.vp, float4(v1.position, 1.0));
    
    float2 p0_ndc = p0_clip.xy / p0_clip.w;
    float2 p1_ndc = p1_clip.xy / p1_clip.w;
    
    float2 line_dir = normalize(p1_ndc - p0_ndc);
    float2 line_perp = float2(-line_dir.y, line_dir.x);
    
    float thickness = 0.003;
    
    float2 offsets[6] = {
        -line_perp * thickness,
         line_perp * thickness,   
         line_perp * thickness,
        -line_perp * thickness,
         line_perp * thickness,
        -line_perp * thickness
    };
    
    bool is_start[6] = { true, true, false, true, false, false };
    
    float4 base_clip = is_start[quad_vertex] ? p0_clip : p1_clip;
    float2 offset = offsets[quad_vertex];
    
    output.position = base_clip;
    output.position.xy += offset * base_clip.w;
    
    output.color = is_start[quad_vertex] ? v0.color : v1.color;
    
    return output;
}

[shader("fragment")]
float4 fragment_main(DebugVertex input) : SV_Target
{
    return input.color;
}
