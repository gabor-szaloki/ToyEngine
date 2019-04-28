#ifndef SHADOW_INCLUDED
#define SHADOW_INCLUDED

#include "Input.hlsl"
#include "ShadowSamplingTent.hlsl"

#define SOFT_SHADOWS
#ifdef SOFT_SHADOWS
#define SOFT_SHADOW_SAMPLES 9
#define SOFT_SHADOW_SAMPLES_FUNC SampleShadow_ComputeSamples_Tent_5x5
#endif

float SampleMainLightShadow(float4 lightSpacePos)
{
	float2 shadowmapUV;
	shadowmapUV.x = 0.5f + (lightSpacePos.x / lightSpacePos.w * 0.5f);
	shadowmapUV.y = 0.5f - (lightSpacePos.y / lightSpacePos.w * 0.5f);

	float pixelDepth = lightSpacePos.z / lightSpacePos.w;

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