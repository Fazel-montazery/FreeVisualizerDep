struct Input {
	float3 Position : TEXCOORD0;
};

struct Output {
	float4 Position : SV_POSITION;
};

Output main(Input input) 
{
	Output output;
	output.Position = float4(input.Position, 1.0);
	return output;
}
