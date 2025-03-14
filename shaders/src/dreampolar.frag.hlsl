#define FOV 1.0

#define MAX_MARCHING_STEPS 15
#define MIN_DISTANCE 0.01
#define MAX_TOTAL_DISTANCE 10.0

#define DOUBLE_PI 6.28318530718

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
        float4 position : SV_Position;
};

struct Ray
{
        float3 pos;
        float3 dir;
        float3 color;
};

float2x2 rot2D(float angle)
{
        float s = sin(angle);
        float c = cos(angle);
        return float2x2(c, -s, s, c);
}

float hash(float3 p)
{
	p = frac(p * 0.3183099f + 0.5);
	p *= 0.3183099f;
	return frac(p.x + p.y + p.z);
}

// Compute Perlin noise at a given point
float perlinNoise(float3 p)
{
	// Determine grid cell coordinates
	float3 i = floor(p);
	float3 fr = frac(p);

	// Compute fade curves for interpolation
	float3 u = fr * fr * (3.0f - 2.0f * fr);

	// Hash the corner points
	float a = hash(i);
	float b = hash(i + float3(1.0f, 0.0f, 0.0f));
	float c = hash(i + float3(0.0f, 1.0f, 0.0f));
	float d = hash(i + float3(1.0f, 1.0f, 0.0f));
	float e = hash(i + float3(0.0f, 0.0f, 1.0f));
	float f = hash(i + float3(1.0f, 0.0f, 1.0f));
	float g = hash(i + float3(0.0f, 1.0f, 1.0f));
	float h = hash(i + float3(1.0f, 1.0f, 1.0f));

	// Interpolate between the corner values
	return lerp(
		lerp(
			lerp(a, b, u.x),
			lerp(c, d, u.x),
			u.y
		),
		lerp(
			lerp(e, f, u.x),
			lerp(g, h, u.x),
			u.y
		),
		u.z
	);
}

float polarSDF(float3 p, float radius)
{
	float noise = (perlinNoise(p * 2.0 + Time) + 1.0) * 0.5;
        return length(p) - radius * (noise);
}

float march(float3 p)
{
	float3 pPos = p;
	pPos.xz = mul(pPos.xz, rot2D(Time));
        float polar = polarSDF(pPos, 1.0 + PeakAmp);
        return polar;
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

        float2 mouse = (Mouse * 2.0 - Resolution) / Resolution.y;
        mouse.y = -mouse.y;

        // Setting the ray
        Ray ray;
        ray.pos = float3(0.0, 0.0, 3.0);
        ray.dir = normalize(float3(uv * FOV, -1.0));

        float t = 0.0;

        for (int i = 0; i < MAX_MARCHING_STEPS; i++) {
                float3 p = ray.pos + ray.dir * t;

                float d = march(p);
                t += d;

                ray.color = t;

                if (d <= MIN_DISTANCE) break;
                if (t > MAX_TOTAL_DISTANCE) break;
        }

        // Main part
        return float4(ray.color * 0.2 * palette(length(uv), color1, color2, color3, color4), 1.0);
}
