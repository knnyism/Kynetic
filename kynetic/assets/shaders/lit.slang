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
uniform Sampler2D<float4> display_texture;

[[vk::push_constant]]
DrawPushConstants constants;

[shader("vertex")]
VertexStageOutput vertex_main(uint vertex_id : SV_VertexID)
{
    VertexStageOutput output;

    Vertex* vertices = (Vertex*)constants.vertex_buffer;
    Vertex v = vertices[vertex_id];

    float4x4 mvp = mul(scene_data.vp, constants.model);

    output.coarseVertex.color = v.color.xyz;
    output.coarseVertex.uv = float2(v.uv_x, v.uv_y);
    output.sv_position = mul(mvp, float4(v.position, 1.0));

    return output;
}

struct Fragment
{
    float4 color;
};

[shader("fragment")]
Fragment fragment_main(CoarseVertex coarse_vertex : CoarseVertex) : SV_Target
{
    float3 color = coarse_vertex.color;

    Fragment output;
    output.color = float4(display_texture.Sample(coarse_vertex.uv).xyz, 1.0);
    return output;
}