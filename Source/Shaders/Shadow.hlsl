#ifndef SHADOW_INCLUDED
#define SHADOW_INCLUDED

#include "ConstantBuffers.hlsl"
#include "ShadowSamplingTent.hlsl"

Texture2D<float> _MainLightShadowmap : register(t2);
SamplerComparisonState _ShadowCmpSampler : register(s2);

#define SOFT_SHADOWS
#ifdef SOFT_SHADOWS
#define SOFT_SHADOW_SAMPLES 9
#define SOFT_SHADOW_SAMPLES_FUNC SampleShadow_ComputeSamples_Tent_5x5
#endif

float SampleMainLightShadow(float4 shadowCoords)
{
	float2 shadowmapUV = shadowCoords.xy / shadowCoords.w;
	float pixelDepth = shadowCoords.z / shadowCoords.w;

	// This would be needed if the texture uv scale-bias would not be precalculated in the shadow matrix to save a MAD here
	//shadowmapUV.x = 0.5f + shadowmapUV.x * 0.5f;
	//shadowmapUV.y = 0.5f - shadowmapUV.y * 0.5f;

#ifdef SOFT_SHADOWS
	float fetchesWeights[SOFT_SHADOW_SAMPLES];
	float2 fetchesUV[SOFT_SHADOW_SAMPLES];
	SOFT_SHADOW_SAMPLES_FUNC(_MainLightShadowResolution, shadowmapUV, fetchesWeights, fetchesUV);
	float average = 0;
	for (int i = 0; i < SOFT_SHADOW_SAMPLES; i++)
		average += _MainLightShadowmap.SampleCmpLevelZero(_ShadowCmpSampler, fetchesUV[i], pixelDepth) * fetchesWeights[i];
	return average;
#else
	return _MainLightShadowmap.SampleCmpLevelZero(_ShadowCmpSampler, shadowmapUV, pixelDepth);
#endif
}

#endif