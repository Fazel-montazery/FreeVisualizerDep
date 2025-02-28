cbuffer UniformBlock : register(b0, space3)
{
	float2 Resolution; // Window size
	float2 Mouse; // Mouse position
	float Time; // Time in sec
	float PeakAmp; // Peak amplitude of music
	float AvgAmp; // Avg amplitude of music
};

struct Input
{
	float4 position : SV_Position; // Fragment position
};

float4 main(Input input) : SV_Target0
{
	// Normalizing Resolution
	float2 uv = (input.position.xy * 2.0 - Resolution) / Resolution.y;
	uv.y = -uv.y;

	// Normalizing Mouse position
        float2 mouse = (Mouse * 2.0 - Resolution) / Resolution.y;
        mouse.y = -mouse.y;

	return 1.0;
}
