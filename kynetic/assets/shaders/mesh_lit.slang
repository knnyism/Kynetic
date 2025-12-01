struct VertexOutput
{
    float4 position : SV_Position;
}

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(1, 1, 1)]
void mesh_main(out indices uint3 triangles[1], out vertices VertexOutput vertices[3], uint3 dispatch_id : SV_DispatchThreadID) 
{
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
