#include "shader_types.hpp"

static constexpr float PI = 3.14159265358979323846f;
static constexpr float EPSILON = 1e-6f;
static constexpr float GOLDEN_RATIO = 0.618033988749895f;

// LOD level color palette for debugging
static const float3 LOD_COLORS[8] = {
    float3(0.0, 1.0, 0.0),   // LOD 0 - Green (original)
    float3(0.0, 0.8, 0.4),   // LOD 1
    float3(0.0, 0.6, 0.8),   // LOD 2
    float3(0.0, 0.4, 1.0),   // LOD 3
    float3(0.4, 0.2, 1.0),   // LOD 4
    float3(0.8, 0.0, 1.0),   // LOD 5
    float3(1.0, 0.0, 0.6),   // LOD 6
    float3(1.0, 0.0, 0.0),   // LOD 7+ - Red (coarsest)
};

float3 get_lod_color(uint lod_level)
{
    uint idx = min(lod_level, 7);
    return LOD_COLORS[idx];
}

float3 get_pastel_color(uint index)
{
    float hue = frac(float(index) * GOLDEN_RATIO);
    float saturation = 0.4f + 0.2f * frac(float(index) * 0.317f);
    float value = 0.85f + 0.15f * frac(float(index) * 0.513f);

    float h = hue * 6.0f;
    float c = value * saturation;
    float x = c * (1.0f - abs(fmod(h, 2.0f) - 1.0f));
    float m = value - c;

    float3 rgb;
    if (h < 1.0f) rgb = float3(c, x, 0.0f);
    else if (h < 2.0f) rgb = float3(x, c, 0.0f);
    else if (h < 3.0f) rgb = float3(0.0f, c, x);
    else if (h < 4.0f) rgb = float3(0.0f, x, c);
    else if (h < 5.0f) rgb = float3(x, 0.0f, c);
    else rgb = float3(c, 0.0f, x);

    return rgb + float3(m, m, m);
}

float ggx(const float roughness, const float n_dot_h)
{
    const float one_minus_n_dot_h_squared = 1.0 - n_dot_h * n_dot_h;

    const float a = n_dot_h * roughness;
    const float k = roughness / (one_minus_n_dot_h_squared + a * a);
    return k * (k * (1.0 / PI));
}

float3 schlick(const float3 f0, float v_dot_h)
{
    float f = pow(1.0 - v_dot_h, 5.0);
    return f + f0 * (1.0 - f);
}

float smith_ggx_correlated(float n_dot_v, float n_dot_l, float roughness)
{
    float a2 = roughness * roughness;
    float lambda_v = n_dot_l * sqrt((n_dot_v - a2 * n_dot_v) * n_dot_v + a2);
    float lambda_l = n_dot_v * sqrt((n_dot_l - a2 * n_dot_l) * n_dot_l + a2);
    return 0.5 / (lambda_v + lambda_l);
}

float lambertian() { return 1.0 / PI; }

bool is_visible(float4x4 mat, float3 origin, float radius)
{
    uint plane_index = 0;
    for (uint i = 0; i < 3; ++i)
    {
        for (uint j = 0; j < 2; ++j, ++plane_index)
        {
            if (plane_index == 2 || plane_index == 3)
            {
                continue;
            }
            const float sign = (j > 0) ? 1.f : -1.f;
            float4 plane = 0.f;
            for (uint k = 0; k < 4; ++k)
            {
                plane[k] = mat[3][k] + sign * mat[i][k];
            }
            plane.xyzw /= sqrt(dot(plane.xyz, plane.xyz));
            if (dot(origin, plane.xyz) + plane.w + radius < 0)
            {
                return false;
            }
        }
    }
    return true;
}

bool cone_cull(float3 center, float radius, float3 cone_axis, float cone_cutoff, float3 camera_position)
{
    return dot(center - camera_position, cone_axis) >= cone_cutoff * length(center - camera_position) + radius;
}

bool project_sphere(float3 C, float r, float znear, float P00, float P11, out float4 aabb) {
    if (-C.z - r < znear) {
        aabb = 0.0;
        return false;
    }

    float2 cx = float2(C.x, -C.z);
    float2 vx = float2(sqrt(dot(cx, cx) - r * r), r);
    float2 minx = mul(float2x2(vx.x, vx.y, -vx.y, vx.x), cx);
    float2 maxx = mul(float2x2(vx.x, -vx.y, vx.y, vx.x), cx);

    float2 cy = -C.yz;
    float2 vy = float2(sqrt(dot(cy, cy) - r * r), r);
    float2 miny = mul(float2x2(vy.x, vy.y, -vy.y, vy.x), cy);
    float2 maxy = mul(float2x2(vy.x, -vy.y, vy.y, vy.x), cy);

    aabb = float4(minx.x / minx.y * P00, miny.x / miny.y * P11, maxx.x / maxx.y * P00, maxy.x / maxy.y * P11);
    aabb = aabb.xwzy * float4(0.5f, -0.5f, 0.5f, -0.5f) + float4(0.5f); // clip space -> uv space

    return true;
}
