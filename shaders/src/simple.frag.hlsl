#define DOUBLE_PI 6.28318530718

cbuffer UniformBlock : register(b0, space3)
{
        float2 Resolution;
        float Time;
        float PeakAmp;
        float AvgAmp;
};

struct Input
{
        float4 position : SV_Position;
};

float sdEquilateralTriangle(float2 p, float r)
{
        const float k = sqrt(3.0);
        p.x = abs(p.x) - r;
        p.y = p.y + r/k;
        if( p.x+k*p.y>0.0 ) p = float2(p.x-k*p.y,-k*p.x-p.y)/2.0;
        p.x -= clamp( p.x, -2.0*r, 0.0 );
        return -length(p)*sign(p.y);
}

static const float3 color1 = float3(0.610, 0.498, 0.650);
static const float3 color2 = float3(0.388, 0.498, 0.350);
static const float3 color3 = float3(0.530, 0.498, 0.620);
static const float3 color4 = float3(3.438, 3.012, 4.025);

float3 palette(float t, float3 a, float3 b, float3 c, float3 d)
{
        return a + b * cos(DOUBLE_PI * (c*t+d));
}

float4 main(Input input) : SV_Target0
{
        float2 uv = (input.position.xy * 2.0 - Resolution) / Resolution.y;
        uv.y = -uv.y;
        float2 uv0 = uv;
        float3 finalColor = 0.0;

        // Main part

        for (int i = 0.0; i < 4.0; i++) { 
                uv = frac(uv * 2.0) - 0.5;
                float3 color = sdEquilateralTriangle(uv, 0.2) * exp(-length(uv0));
                color = sin(color * 10.0 + (1.0 - AvgAmp) * 6.0)/10.0;
                color = abs(color);
                color = smoothstep(0.0, 0.1, color);
                color = pow(0.1 / color, 1.3);
                finalColor = color * palette(length(uv0) + i * (1.0 - PeakAmp), color1, color2, color3, color4);
        }

	return float4(finalColor, 1.0);
}
