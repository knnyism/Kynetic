#include "shader_shared.slang"

struct VkDrawIndexedIndirectCommand 
{
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t vertexOffset;
    uint32_t firstInstance;
};

ConstantBuffer<SceneData> scene;

StructuredBuffer<InstanceData> instances_in; 
RWStructuredBuffer<InstanceData> instances_out;

[[vk::push_constant]] FrustumCullPushConstants constants;

[shader("compute")]
[numthreads(64, 1, 1)]
void main(uint3 global_id : SV_DispatchThreadID)
{
    uint instance_id = global_id.x;
    if (instance_id >= constants.instance_count) return;
    
    VkDrawIndexedIndirectCommand* draws = (VkDrawIndexedIndirectCommand*)constants.draw_commands;

    InstanceData instance = instances_in[instance_id];

    float4[6] cull_frustum = scene.use_debug_culling > 0 ? scene.debug_frustum : scene.frustum;

    float4 center = mul(instance.model, float4(instance.position.xyz, 1.0f));

    float scale_x = length(float3(instance.model[0][0], instance.model[1][0], instance.model[2][0]));
    float scale_y = length(float3(instance.model[0][1], instance.model[1][1], instance.model[2][1]));
    float scale_z = length(float3(instance.model[0][2], instance.model[1][2], instance.model[2][2]));
    float scale = max(scale_x, max(scale_y, scale_z));
    float radius = instance.position.w * scale;

    if (is_visible(cull_frustum, center.xyz, radius))
    {
        uint draw_id = instance.draw_id;
        uint base = draws[draw_id].firstInstance;
        
        uint local_index;
        InterlockedAdd(draws[draw_id].instanceCount, 1, local_index);
        
        instances_out[base + local_index] = instance;
    }
}