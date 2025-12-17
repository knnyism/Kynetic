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

[shader("fragment")]
Fragment fragment_main(CoarseVertex coarse_vertex : CoarseVertex) : SV_Target
{
    Fragment output;

    float3 albedo = get_pastel_color(coarse_vertex.material_index);

    float3 light_direction = -scene.sun_direction.xyz;
    float3 light_color = scene.sun_color.xyz;

    float n_dot_l = saturate(dot(coarse_vertex.normal, -scene.sun_direction.xyz));

    output.color = float4(albedo * n_dot_l * light_color, 1.0);
    return output;
}
