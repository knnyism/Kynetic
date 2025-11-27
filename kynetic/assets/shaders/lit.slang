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
    float4       sv_position  : SV_Position;
};

ConstantBuffer<SceneData>               scene;

[[vk::binding(0, 1)]] Sampler2D         textures[];
[[vk::push_constant]] DrawPushConstants constants;

[shader("vertex")]
VertexStageOutput vertex_main(uint vertex_id : SV_VulkanVertexID, uint instance_id : SV_VulkanInstanceID)
{
    VertexStageOutput output;

    Vertex* vertices = (Vertex*)constants.vertices;
    InstanceData* instances = (InstanceData*)constants.instances;

    Vertex v = vertices[vertex_id];
    InstanceData instance = instances[instance_id];

    float4x4 mvp = mul(scene.vp, instance.model);
    float3x3 model_inv = float3x3(instance.model_inv);

    output.coarse_vertex.color = v.color.xyz;
    output.coarse_vertex.uv = float2(v.uv_x, v.uv_y);

    output.coarse_vertex.tangent = normalize(mul(model_inv, v.tangent.xyz));
    output.coarse_vertex.bitangent = normalize(mul(model_inv, cross(v.normal, v.tangent.xyz) * v.tangent.w));
    output.coarse_vertex.normal = normalize(mul(model_inv, v.normal));

    output.coarse_vertex.world_position = mul(instance.model, float4(v.position, 1.0)).xyz;

    output.coarse_vertex.material_index = instance.material_index;

    output.sv_position = mul(mvp, float4(v.position, 1.0));

    return output;
}

struct Fragment
{
    float4 color;
};

float3 evaluate_light(float3 albedo, float3 V, float3 N, float NoV, float NoL, float roughness, float metallic, float3 f0, float3 L, float3 radiance)
{
    float3 H = normalize(V + L);

    float NoH = saturate(dot(N, H));
    float LoH = saturate(dot(L, H));

    float3 F = schlick(f0, LoH);
    float D = ggx(roughness, NoH);
    float G = smith_ggx_correlated(NoV, NoL, roughness);

    float3 Fd = albedo * (1.0 - F) * (1.0 - metallic) * lambertian();
    float3 Fr = F * D * G;

    return (Fd + Fr) * radiance * NoL;
}

[shader("fragment")]
Fragment fragment_main(CoarseVertex coarse_vertex : CoarseVertex) : SV_Target
{
    Fragment output;

    MaterialData* materials = (MaterialData*)constants.materials;
    MaterialData material = materials[coarse_vertex.material_index];

    float3 albedo = textures[material.albedo].Sample(coarse_vertex.uv).xyz;
    float3 normal_map = textures[material.normal].Sample(coarse_vertex.uv).xyz * 2.0f - 1.0f;
    float3 metal_rough = textures[material.metal_rough].Sample(coarse_vertex.uv).xyz;
    float3 emissive = textures[material.emissive].Sample(coarse_vertex.uv).xyz;

    float3x3 tbn = float3x3(coarse_vertex.tangent, coarse_vertex.bitangent, coarse_vertex.normal);

    float3 shading_normal = normalize(mul(normal_map, tbn));
    float3 shading_view = normalize(scene.view_inv[3].xyz - coarse_vertex.world_position);

    float linear_roughness = max(metal_rough.g, EPSILON);
    float roughness = linear_roughness * linear_roughness;
    float metallic = metal_rough.b;

    float n_dot_v = max(abs(dot(shading_normal, shading_view)), EPSILON);
    float3 f0 = lerp(0.04, albedo, metallic);

    float3 Lo = 0.0;

    float3 light_direction = -scene.sun_direction.xyz;
    float3 light_color = scene.sun_color.xyz;

    float n_dot_l = saturate(dot(shading_normal, -scene.sun_direction.xyz));
    Lo += evaluate_light(albedo, shading_view, shading_normal, n_dot_v, n_dot_l,
                                                                        roughness,
                                                                        metallic,
                                                                        f0,
                                                                        light_direction,
                                                                        light_color);

    Lo += emissive;

    RenderChannel channel = RenderChannel::Final;
    switch (channel)
    {
        case RenderChannel::Final:              output.color = float4(Lo, 1.0); break;
        case RenderChannel::TexCoords:          output.color = float4(coarse_vertex.uv.x, coarse_vertex.uv.y, 0.0, 1.0); break;
        case RenderChannel::NormalTexture:      output.color = float4(normal_map * 0.5 + 0.5, 1.0); break;
        case RenderChannel::GeometryTangent:    output.color = float4(tbn[0] * 0.5 + 0.5, 1.0); break;
        case RenderChannel::GeometryBitangent:  output.color = float4(tbn[1] * 0.5 + 0.5, 1.0); break;
        case RenderChannel::GeometryNormal:     output.color = float4(tbn[2] * 0.5 + 0.5, 1.0); break;
        case RenderChannel::ShadingNormal:      output.color = float4(shading_normal * 0.5 + 0.5, 1.0); break;
        case RenderChannel::BaseColor:          output.color = float4(albedo, 1.0); break;
        case RenderChannel::Metallic:           output.color = float4(metallic, metallic, metallic, 1.0); break;
        case RenderChannel::Roughness:          output.color = float4(linear_roughness, linear_roughness, linear_roughness, 1.0); break;
        case RenderChannel::Emissive:           output.color = float4(emissive, 1.0); break;
        // case RenderChannel::Occlusion:		ouput.color = float4(occlusion, occlusion, occlusion, 1.0); break;
        default: output.color = float4(1.0, 0.0, 1.0, 1.0); break;
    }

    return output;
}
