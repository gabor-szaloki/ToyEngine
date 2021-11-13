#define POSTFX_NEED_UV 1
#include "PostFxCommon.hlsl"

Texture2D<float> _ShadowMap : register(t0);
SamplerState _Sampler : register(s0);

#define BLUR_RADIUS (1)
#define BLUR_SIZE ((BLUR_RADIUS * 2 + 1))
#define BLUR_NUM_SAMPLES (BLUR_SIZE * BLUR_SIZE)

float2 ComputeMoments(float depth)
{
	float2 moments;

	// First moment is the depth itself.
	moments.x = depth;

	// Compute partial derivatives of depth.
	float dx = ddx(depth);
	float dy = ddy(depth);

	// Compute second moment over the pixel extents.
	moments.y = depth * depth + 0.25 * (dx * dx + dy * dy);
	return moments;
}

float2 VarianceShadowMapPS(DefaultPostFxVsOutput i) : SV_TARGET
{
	float2 momentsAcc = 0;

	[unroll] for (int y = -BLUR_RADIUS; y <= BLUR_RADIUS; y++)
		[unroll] for (int x = -BLUR_RADIUS; x <= BLUR_RADIUS; x++)
			momentsAcc += ComputeMoments(_ShadowMap.SampleLevel(_Sampler, i.uv, 0, int2(x, y)));

	return momentsAcc / BLUR_NUM_SAMPLES;
}