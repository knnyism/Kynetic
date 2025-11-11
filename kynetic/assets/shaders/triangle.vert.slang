struct AssembledVertex
{
    float3	position : POSITION;
    float3	color    : COLOR;
};

struct CoarseVertex
{
    float3 color;
};

struct VertexStageOutput
{
    CoarseVertex    coarseVertex    : CoarseVertex;
    float4          sv_position     : SV_Position;
};

[shader("vertex")]
VertexStageOutput main(
    AssembledVertex assembledVertex, uint vertexId : SV_VertexID)
{
    VertexStageOutput output;
    const float3 positions[3] = float3[3](
		float3(1.f,1.f, 0.0f),
		float3(-1.f,1.f, 0.0f),
		float3(0.f,-1.f, 0.0f)
	);

	const float3 colors[3] = float3[3](
		float3(1.0f, 0.0f, 0.0f), //red
		float3(0.0f, 1.0f, 0.0f), //green
		float3(0.f, 0.0f, 1.0f)  //blue
	);

    // float3 position = assembledVertex.position;
    // float3 color = assembledVertex.color;

    // output.coarseVertex.color = color;
    // output.sv_position = mul(modelViewProjection, float4(position, 1.0));

    output.coarseVertex.color = colors[vertexId];
    output.sv_position = float4(positions[vertexId], 1.0f);
    
    return output;
}
