#include "shader_shared.slang"

[[vk::binding(0, 0)]]
RWTexture2D<float4> image;

ConstantBuffer<SceneData> scene;

static const float3 SKY_COLOR_TOP = float3(0.4, 0.4, 0.4);  // Beige
static const float3 SKY_COLOR_HORIZON = float3(0.75, 0.75, 0.75);  // Light gray
static const float3 SKY_COLOR_BOTTOM = float3(0.96, 0.94, 0.88);     // Gray

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 global_id : SV_DispatchThreadID)
{
    int2 coords = int2(global_id.xy);
    int2 size;
    image.GetDimensions(size.x, size.y);
    
    if (coords.x < size.x && coords.y < size.y)
    {
        float2 uv = (float2(coords) + 0.5) / float2(size);
        
        float2 ndc = uv * 2.0 - 1.0;
        ndc.y = -ndc.y;
        
        float3 cam_right   = scene.view_inv[0].xyz;
        float3 cam_up      = scene.view_inv[1].xyz;
        float3 cam_forward = scene.view_inv[2].xyz;
        
        float tan_half_fov_y = 1.0 / scene.projection_11;
        float tan_half_fov_x = 1.0 / scene.projection_00;
        
        float3 world_dir = normalize(
            cam_forward + 
            cam_right * (ndc.x * tan_half_fov_x) + 
            cam_up * (ndc.y * tan_half_fov_y)
        );
        
        float t = world_dir.y;
        
        float3 color;
        if (t >= 0.0)
        {
            float sky_t = smoothstep(0.0, 0.8, t);
            color = lerp(SKY_COLOR_HORIZON, SKY_COLOR_TOP, sky_t);
        }
        else
        {
            float ground_t = smoothstep(0.0, -0.8, t);
            color = lerp(SKY_COLOR_HORIZON, SKY_COLOR_BOTTOM, ground_t);
        }
        
        image[coords] = float4(color, 1.0);
    }
}
