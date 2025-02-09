static const float2 positions[4] = {
	float2(-1.0,  -1.0),
	float2(1.0, -1.0),
	float2(-1.0, 1.0),
	float2(1.0, 1.0)
};

struct Output {
	float4 Position : SV_POSITION;
};

Output main(uint index: SV_VertexID) 
{
	Output output;
	output.Position = float4(positions[index], 0.0, 1.0);
	return output;
}
