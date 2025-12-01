[[vk::binding(0, 0)]]
RWTexture2D<float4> image;

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 global_id : SV_DispatchThreadID)
{
    int2 coords = int2(global_id.xy);

    int2 size;
    image.GetDimensions(size.x, size.y);

    if (coords.x < size.x && coords.y < size.y) image[coords] = float4(0.0);
}