#include "shader_types.hpp"

struct CoarseVertex
{
    float3 color;
};

struct VertexStageOutput
{
    CoarseVertex    coarseVertex    : CoarseVertex;
    float4          sv_position     : SV_Position;
};

[[vk::push_constant]]
DrawPushConstants constants;

[shader("vertex")]
VertexStageOutput main(uint vertex_id : SV_VertexID)
{
    VertexStageOutput output;

    // Cast the address to a buffer reference
    Vertex* vertices = (Vertex*)constants.vertex_buffer;
    Vertex v = vertices[vertex_id];

    output.coarseVertex.color = v.color.xyz;
    output.sv_position = mul(constants.world_matrix, float4(v.position, 1.0));

    return output;
}