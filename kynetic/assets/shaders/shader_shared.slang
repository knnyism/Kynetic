#include "shader_types.hpp"

static constexpr float PI = 3.14159265358979323846f;
static constexpr float EPSILON = 1e-6f;

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
