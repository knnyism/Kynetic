#include "shader_types.hpp"

RWTexture2D<float4> image;

[[vk::push_constant]]
GradientPushConstants constants;

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 globalID : SV_DispatchThreadID,
          uint3 localID : SV_GroupThreadID)
{
    int2 texelCoord = int2(globalID.xy);

    float4 topColor = constants.data1;
    float4 bottomColor = constants.data2;

    if (texelCoord.x < constants.size.x && texelCoord.y < constants.size.y)
    {
        float blend = float(texelCoord.y)/(constants.size.y);

        image[texelCoord] = lerp(topColor, bottomColor, blend);
    }
}