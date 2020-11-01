#ifndef TONEMAPPING_INCLUDED
#define TONEMAPPING_INCLUDED

static const float exposure = 1.0f;
static const float A = 0.22f; // Shoulder strength curve parameter
static const float B = 0.30f; // Linear strength curve parameter
static const float C = 0.10f; // Linear angle curve parameter
static const float D = 0.20f; // Toe strength curve parameter
static const float E = 0.01f; // Toe numerator curve parameter
static const float F = 0.30f; // Toe denominator curve parameter
static const float W = 11.2f; // White point\nMinimal value that is mapped to 1.

float3 uncharted_2_tonemap(float3 x)
{
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 tonemap(float3 hdr_color)
{
	float exposureBias = 2.0f;
	float3 curr = exposureBias * uncharted_2_tonemap(exposure * hdr_color);
	float3 whiteScale = 1.0f / uncharted_2_tonemap(W);
	float3 color = curr * whiteScale;
	return saturate(color);
}

#endif