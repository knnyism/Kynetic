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
    VkDrawMeshTasksIndirectCommandEXT* draw_commands = (VkDrawMeshTasksIndirectCommandEXT*)constants.draw_commands;

    draw_commands[id].groupCountX *= is_visible(cull_frustum, instances[id].position.xyz, instances[id].position.w);
}