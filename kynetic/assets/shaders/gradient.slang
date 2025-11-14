#include "shader_types.hpp"

[[vk::binding(0, 0)]]
RWTexture2D<float4> image;

[[vk::push_constant]]
GradientPushConstants pushConstants;

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 globalID : SV_DispatchThreadID,
          uint3 localID : SV_GroupThreadID)
{
    int2 texelCoord = int2(globalID.xy);

    float4 topColor = pushConstants.data1;
    float4 bottomColor = pushConstants.data2;

    if (texelCoord.x < pushConstants.size.x && texelCoord.y < pushConstants.size.y)
    {
        float blend = float(texelCoord.y)/(pushConstants.size.y);

        image[texelCoord] = lerp(topColor, bottomColor, blend);
    }
}