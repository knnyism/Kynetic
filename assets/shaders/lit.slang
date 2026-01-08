#include "shader_shared.slang"

struct CoarseVertex
{
    float3               color;
    float2               uv;
    float3               tangent;
    float3               bitangent;
    float3               normal;
                         
    float3               world_position;
    nointerpolation uint material_index;
};

struct VertexStageOutput
{
    CoarseVertex coarse_vertex : CoarseVertex;
    float4       sv_position   : SV_Position;
};

ConstantBuffer<SceneData>               scene;
[[vk::binding(0, 1)]] Sampler2D         textures[];
[[vk::push_constant]] DrawPushConstants constants;

[shader("vertex")]
VertexStageOutput vertex_main(uint vertex_id : SV_VulkanVertexID, uint instance_id : SV_VulkanInstanceID)
{
    VertexStageOutput output;

    float4* positions = (float4*)constants.positions;
    Vertex* vertices = (Vertex*)constants.vertices;
    InstanceData* instances = (InstanceData*)constants.instances;

    float3 position = positions[vertex_id].xyz;
    Vertex v = vertices[vertex_id];
    InstanceData instance = instances[instance_id];

    float4x4 mvp = mul(scene.vp, instance.model);
    float3x3 model_inv = float3x3(instance.model_inv);

    output.coarse_vertex.color = v.color.xyz;
    output.coarse_vertex.uv = float2(v.uv_x, v.uv_y);

    output.coarse_vertex.tangent = normalize(mul(model_inv, v.tangent.xyz));
    output.coarse_vertex.bitangent = normalize(mul(model_inv, cross(v.normal, v.tangent.xyz) * v.tangent.w));
    output.coarse_vertex.normal = normalize(mul(model_inv, v.normal));

    output.coarse_vertex.world_position = mul(instance.model, float4(position, 1.0)).xyz;
    output.coarse_vertex.material_index = instance.material_index;

    output.sv_position = mul(mvp, float4(position, 1.0));

    return output;
}

struct Fragment
{
    float4 color;
};

static const float3 SH_L00  = float3(0.68, 0.67, 0.64);
static const float3 SH_L1_1 = float3(0.0, 0.0, 0.0);
static const float3 SH_L10  = float3(0.28, 0.27, 0.24);
static const float3 SH_L11  = float3(0.0, 0.0, 0.0);
static const float3 SH_L2_2 = float3(0.0, 0.0, 0.0);
static const float3 SH_L2_1 = float3(0.0, 0.0, 0.0);
static const float3 SH_L20  = float3(0.04, 0.04, 0.03);
static const float3 SH_L21  = float3(0.0, 0.0, 0.0);
static const float3 SH_L22  = float3(0.0, 0.0, 0.0);

float3 evaluate_sh_irradiance_fast(float3 n)
{
    const float A0 = 3.141593; // pi
    const float A1 = 2.094395; // 2*pi/3
    const float A2 = 0.785398; // pi/4

    float3 result = float3(0.0);

    result += SH_L00 * A0 * 0.282095;

    result += SH_L1_1 * A1 * 0.488603 * n.y;
    result += SH_L10  * A1 * 0.488603 * n.z;
    result += SH_L11  * A1 * 0.488603 * n.x;

    result += SH_L2_2 * A2 * 1.092548 * (n.x * n.y);
    result += SH_L2_1 * A2 * 1.092548 * (n.y * n.z);
    result += SH_L20  * A2 * 0.315392 * (3.0 * n.z * n.z - 1.0);
    result += SH_L21  * A2 * 1.092548 * (n.x * n.z);
    result += SH_L22  * A2 * 0.546274 * (n.x * n.x - n.y * n.y);

    return max(result, float3(0.0));
}

float oren_nayar(float3 N, float3 L, float3 V, float roughness)
{
    float sigma2 = roughness * roughness;
    
    float n_dot_l = saturate(dot(N, L));
    float n_dot_v = saturate(dot(N, V));
    
    if (n_dot_l < 1e-6 || n_dot_v < 1e-6)
        return n_dot_l;
    
    float A = 1.0 - 0.5 * (sigma2 / (sigma2 + 0.33));
    float B = 0.45 * (sigma2 / (sigma2 + 0.09));
    
    float theta_i = acos(n_dot_l);
    float theta_r = acos(n_dot_v);
    
    float alpha = max(theta_i, theta_r);
    float beta = min(theta_i, theta_r);
    
    float3 L_proj = normalize(L - N * n_dot_l);
    float3 V_proj = normalize(V - N * n_dot_v);
    
    float cos_phi_diff = max(0.0, dot(L_proj, V_proj));
    
    return n_dot_l * (A + B * cos_phi_diff * sin(alpha) * tan(beta));
}

[shader("fragment")]
Fragment fragment_main(CoarseVertex coarse_vertex : CoarseVertex) : SV_Target
{
    Fragment output;
    
    float3 N = normalize(coarse_vertex.normal);
    float3 L = -normalize(scene.sun_direction.xyz);
    float3 V = normalize(scene.view_inv[3].xyz - coarse_vertex.world_position);
    
    float3 albedo = get_pastel_color(coarse_vertex.material_index);
    float3 light_color = scene.sun_color.xyz;
    
    float roughness = 0.5;
    float diffuse = oren_nayar(N, L, V, roughness);
    float3 direct = albedo * diffuse * light_color;
    
    float3 sh_irradiance = evaluate_sh_irradiance_fast(N);
    float3 indirect = albedo * sh_irradiance * (1.0 / 3.14159265) * 2.0;
    
    float3 final_color = direct + indirect;
    
    output.color = float4(final_color, 1.0);
    return output;
}