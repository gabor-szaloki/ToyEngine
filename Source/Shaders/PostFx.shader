#include "PostFxCommon.hlsl"
#include "Tonemapping.hlsl"

Texture2D _HdrTarget : register(t0);
SamplerState _HdrTarget_Sampler : register(s0);

float4 PostFxPS(DefaultPostFxVsOutput i) : SV_TARGET
{
	float4 hdrSceneColor = _HdrTarget.Sample(_HdrTarget_Sampler, i.uv);
	float3 tonemappedLinearColor = tonemap(hdrSceneColor.rgb);
	float3 srgbColor = pow(tonemappedLinearColor.rgb, 1.0f/2.2f);
	return float4(srgbColor, 1);
}