#ifndef SHADOW_INCLUDED
#define SHADOW_INCLUDED

#include "Common.hlsl"
#include "ConstantBuffers.hlsl"

Texture2D<float> _MainLightShadowmap : register(t2);
SamplerComparisonState _ShadowCmpSampler : register(s2);

#if SOFT_SHADOWS_POISSON
	#include "Random.hlsl"
	#define SOFT_SHADOW_POISSON_SAMPLES 4
	static const float2 _PoissonOffsets[SOFT_SHADOW_POISSON_SAMPLES] = {
		// 1
		//float2(0.402211,  0.126575),
		// 2
		//float2(0.297056,  0.616830),
		//float2(0.298156, -0.001704),
		// 4
		float2(0.019369,  0.395482),
		float2(-0.066918, -0.367739),
		float2(-0.955010,  0.372377),
		float2(0.800057,  0.120602),
	};
#elif SOFT_SHADOWS_9TAP
	#include "ShadowSamplingTent.hlsl"
	#define SOFT_SHADOW_SAMPLES 9
	#define SOFT_SHADOW_SAMPLES_FUNC SampleShadow_ComputeSamples_Tent_5x5
#endif

float SampleMainLightShadow(float4 shadow_coords, float2 screen_uv)
{
	float2 shadowmapUv = shadow_coords.xy / shadow_coords.w;
	float pixelDepth = shadow_coords.z / shadow_coords.w;

	// This would be needed if the texture uv scale-bias would not be precalculated in the shadow matrix to save a MAD here
	//shadowmapUv.x = 0.5f + shadowmapUv.x * 0.5f;
	//shadowmapUv.y = 0.5f - shadowmapUv.y * 0.5f;

#if SOFT_SHADOWS_POISSON
	float randomAngle = nrand(screen_uv, _TimeParams.x) * 2.0 * PI;
	float cosA, sinA;
	sincos(randomAngle, sinA, cosA);
	float2x2 rotationMatrix = {  cosA, sinA,
	                            -sinA, cosA };
	rotationMatrix *= _MainLightShadowParams.z;
	float result = 0;
	[unroll] for (int i = 0; i < SOFT_SHADOW_POISSON_SAMPLES; i++)
	{
		float2 jitteredShadowmapUv = shadowmapUv + mul(_PoissonOffsets[i], rotationMatrix);
		result += _MainLightShadowmap.SampleCmpLevelZero(_ShadowCmpSampler, jitteredShadowmapUv, pixelDepth);
	}
	return result / SOFT_SHADOW_POISSON_SAMPLES;
#elif SOFT_SHADOWS_9TAP
	float fetchesWeights[SOFT_SHADOW_SAMPLES];
	float2 fetchesUV[SOFT_SHADOW_SAMPLES];
	SOFT_SHADOW_SAMPLES_FUNC(_MainLightShadowParams.yyxx, shadowmapUv, fetchesWeights, fetchesUV);
	float average = 0;
	[unroll] for (int i = 0; i < SOFT_SHADOW_SAMPLES; i++)
		average += _MainLightShadowmap.SampleCmpLevelZero(_ShadowCmpSampler, fetchesUV[i], pixelDepth) * fetchesWeights[i];
	return average;
#else
	return _MainLightShadowmap.SampleCmpLevelZero(_ShadowCmpSampler, shadowmapUv, pixelDepth);
#endif
}

#endif