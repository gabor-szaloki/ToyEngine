#include "PostFxCommon.hlsl"
#include "Tonemapping.hlsl"

Texture2D _HdrTarget : register(t0);
SamplerState _HdrTarget_Sampler : register(s0);

float4 PostFxPS(DefaultPostFxVsOutput i) : SV_TARGET
{
	float4 sceneColor = _HdrTarget.Sample(_HdrTarget_Sampler, i.uv);
	sceneColor.rgb = tonemap(sceneColor.rgb);
	return sceneColor;
}