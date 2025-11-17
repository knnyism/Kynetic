#include "shader_types.hpp"

struct CoarseVertex
{
    float3 color;
    float2 uv;
};

struct VertexStageOutput
{
    CoarseVertex    coarseVertex    : CoarseVertex;
    float4          sv_position     : SV_Position;
};

ConstantBuffer<SceneData> scene_data;

[[vk::push_constant]]
DrawPushConstants constants;

[shader("vertex")]
VertexStageOutput main(uint vertex_id : SV_VertexID)
{
    VertexStageOutput output;

    Vertex* vertices = (Vertex*)constants.vertex_buffer;
    Vertex v = vertices[vertex_id];
    
    output.coarseVertex.color = v.color.xyz;
    output.coarseVertex.uv = float2(v.uv_x, v.uv_y);
    output.sv_position = mul(scene_data.vp, float4(v.position, 1.0));

    return output;
}