#include "shader_shared.slang"

struct VkDrawIndexedIndirectCommand 
{
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t vertexOffset;
    uint32_t firstInstance;
}

ConstantBuffer<SceneData> scene;
RWStructuredBuffer<InstanceData> instances;

[[vk::push_constant]] FrustumCullPushConstants constants;

[shader("compute")]
[numthreads(64, 1, 1)]
void main(uint3 global_id : SV_DispatchThreadID)
{
    uint id = global_id.x;

    if (id >= constants.draw_count) return;

    VkDrawIndexedIndirectCommand* draw_commands = (VkDrawIndexedIndirectCommand*)(constants.draw_commands);

    VkDrawIndexedIndirectCommand draw_command = draw_commands[id];
    uint32_t write_index = draw_command.firstInstance;

    for (uint32_t read_index = draw_command.firstInstance;
         read_index < draw_command.firstInstance + draw_command.instanceCount; 
                  ++read_index)
    {
        InstanceData read_instance = instances[read_index];

        if (is_visible(scene.vp, read_instance.position.xyz, read_instance.position.w)) {
            if (write_index != read_index) instances[write_index] = read_instance;
            write_index++;
        }
    }

    draw_command.instanceCount = write_index - draw_command.firstInstance;
    draw_commands[id] = draw_command;
}