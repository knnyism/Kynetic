
Sampler2D           src;
RWTexture2D<float4> dst;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 global_id : SV_DispatchThreadID)
{
    int2 texel00 = int2(global_id.xy) * 2;
    int2 texel01 = texel00 + int2(0, 1);
    int2 texel10 = texel00 + int2(1, 0);
    int2 texel11 = texel00 + int2(1, 1);

    float color00 = src.Load(int3(texel00, 0)).r;
    float color01 = src.Load(int3(texel01, 0)).r;
    float color10 = src.Load(int3(texel10, 0)).r;
    float color11 = src.Load(int3(texel11, 0)).r;

    // min cuz reverse-z
    float result = min(min(min(color00, color01), color10), color11);

    dst[global_id.xy] = float4(result, 0.0, 0.0, 0.0);

    GroupMemoryBarrier();
}