#include "shader_shared.slang"

struct VkDrawIndexedIndirectCommand 
{
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t vertexOffset;
    uint32_t firstInstance;
}

ConstantBuffer<SceneData>                      scene;
RWStructuredBuffer<InstanceData>               instances;

[[vk::push_constant]] FrustumCullPushConstants constants;

[shader("compute")]
[numthreads(64, 1, 1)]
void main(uint3 global_id : SV_DispatchThreadID)
{
}