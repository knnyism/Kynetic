#include "shader_shared.slang"

struct VertexOutput
{
    float4 position : SV_Position;
}

ConstantBuffer<SceneData>                   scene;
[[vk::push_constant]] MeshDrawPushConstants constants;

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(1, 1, 1)]
void mesh_main(out indices uint3 triangles[124], 
               out vertices VertexOutput vertices[64],
               uint group_id: SV_GroupID,
               uint thread_id: SV_GroupThreadID,
               uint3 dispatch_id: SV_DispatchThreadID)
{
    uint32_t*           indices   = (uint32_t*)constants.indices;
    float4*             positions = (float4*)constants.positions;
    MeshletData*        meshlets  = (MeshletData*)constants.meshlets;

    SetMeshOutputCounts(3, 1);
    vertices[0].position = float4(0.5, -0.5, 0, 1);
    vertices[1].position = float4(0.5, 0.5, 0, 1);
    vertices[2].position = float4(-0.5, 0.5, 0, 1);
    triangles[0] = uint3(0, 1, 2);
}

struct Fragment
{
    float4 color;
};

[shader("fragment")]
Fragment fragment_main() : SV_Target
{
    Fragment output;
    output.color = float4(1.0, 0.0, 0.0, 1.0);

    return output;
}
