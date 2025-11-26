#include "shader_shared.slang"

struct VkDrawIndexedIndirectCommand 
{
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t vertexOffset;
    uint32_t firstInstance;
}

bool is_visible(InstanceData instance)
{
    return true;
}

[[vk::push_constant]] FrustumCullPushConstants constants;

[shader("compute")]
[numthreads(64, 1, 1)]
void main(uint3 global_id : SV_DispatchThreadID)
{
    uint id = global_id.x;

    if (id >= constants.draw_count) return;

    InstanceData* instances = (InstanceData*)(constants.instances);
    VkDrawIndexedIndirectCommand* draw_commands = (VkDrawIndexedIndirectCommand*)(constants.draw_commands);

    VkDrawIndexedIndirectCommand draw_command = draw_commands[id];

    uint32_t last_instance_id = draw_command.firstInstance + draw_command.instanceCount;
    for (uint32_t instance_id = draw_command.firstInstance; instance_id < last_instance_id; ++instance_id)
    {
        if (!is_visible(instances[instance_id])) {
            draw_command.instanceCount -= 1;
            // if (draw_command.instanceCount > 0) instances[instance_id] = instances[last_instance_id];
        }
    }
}