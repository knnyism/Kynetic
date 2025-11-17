#include "shader_types.hpp"

struct CoarseVertex
{
    float3 color;
    float2 uv;
};

struct Fragment
{
    float4 color;
};

[[vk::binding(1,0)]]
uniform Sampler2D<float4> display_texture;

[shader("fragment")]
Fragment main(CoarseVertex coarse_vertex : CoarseVertex) : SV_Target
{
    float3 color = coarse_vertex.color;

    Fragment output;
    output.color = float4(display_texture.Sample(coarse_vertex.uv).xyz, 1.0);
    //output.color = float4(coarse_vertex.color.xyz, 1.0);
    return output;
}