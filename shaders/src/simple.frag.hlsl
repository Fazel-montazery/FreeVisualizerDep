cbuffer UniformBlock : register(b0, space3)
{
        float2 Resolution;
        float Time;
};

struct Input
{
        float4 position : SV_Position;
};

static const float3 color1 = float3(0.0, 1.0, 1.0);
static const float3 color2 = float3(0.639, 0.047, 0.047);

float4 main(Input input) : SV_Target0
{
        float2 uv = input.position.xy / Resolution;
        uv.y = 1.0 - uv.y; // Convert to openGL style coords :)

        // Main part
        float3 pct;
        pct.x = uv.x;
        pct.y = sin(uv.x);
        pct.z = distance(uv, float2(0.5, 0.5)) * abs(sin(Time)) * 2.0;
        float3 color = lerp(color1, color2, pct);
	return float4(color, 1.0);
}
