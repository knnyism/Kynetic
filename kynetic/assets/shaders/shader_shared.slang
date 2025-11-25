#include "shader_types.hpp"

static constexpr float PI = 3.14159265358979323846f;
static constexpr float EPSILON = 1e-6f;

float ggx(const float roughness, const float n_dot_h) {
	const float one_minus_n_dot_h_squared = 1.0 - n_dot_h * n_dot_h;

	const float a = n_dot_h * roughness;
    const float k = roughness / (one_minus_n_dot_h_squared + a * a);
    return k * (k * (1.0 / PI));
}

float3 schlick(const float3 f0, float v_dot_h) {
    float f = pow(1.0 - v_dot_h, 5.0);
    return f + f0 * (1.0 - f);
}

float smith_ggx_correlated(float n_dot_v, float n_dot_l, float roughness) {
    float a2 = roughness * roughness;
    float lambda_v = n_dot_l * sqrt((n_dot_v - a2 * n_dot_v) * n_dot_v + a2);
    float lambda_l = n_dot_v * sqrt((n_dot_l - a2 * n_dot_l) * n_dot_l + a2);
    return 0.5 / (lambda_v + lambda_l);
}

float lambertian() {
    return 1.0 / PI;
}