#include "shader_shared.slang"

struct VkDrawMeshTasksIndirectCommandEXT {
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;
};

ConstantBuffer<SceneData>                      scene;
RWStructuredBuffer<InstanceData>               instances;

[[vk::push_constant]] FrustumCullPushConstants constants;

[shader("compute")]
[numthreads(64, 1, 1)]
void main(uint3 group_thread_id : SV_DispatchThreadID)
{
    uint id = group_thread_id.x;

    if (id >= constants.draw_count) return;

    float4[6] cull_frustum = scene.use_debug_culling > 0 ? scene.debug_frustum : scene.frustum;

    InstanceData instance = instances[id];

    float4 center = mul(instance.model, float4(instance.position.xyz, 1.0f));

    float scale_x = length(float3(instance.model[0][0], instance.model[1][0], instance.model[2][0]));
    float scale_y = length(float3(instance.model[0][1], instance.model[1][1], instance.model[2][1]));
    float scale_z = length(float3(instance.model[0][2], instance.model[1][2], instance.model[2][2]));
    float scale = max(scale_x, max(scale_y, scale_z));
    float radius = instance.position.w * scale;

    VkDrawMeshTasksIndirectCommandEXT* draw_commands = (VkDrawMeshTasksIndirectCommandEXT*)constants.draw_commands;

    draw_commands[id].groupCountX *= is_visible(cull_frustum, center.xyz, radius);
}