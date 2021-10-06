#define POSTFX_NEED_UV 1

#include "PostFxCommon.hlsl"

Texture2D _SrcTex : register(t0);
SamplerState _SrcSampler : register(s0);

float4 BlitPS(DefaultPostFxVsOutput i) : SV_TARGET
{
	return _SrcTex.Sample(_SrcSampler, i.uv);
}

float4 BlitLinearToSrgb(DefaultPostFxVsOutput i) : SV_TARGET
{
	float4 linearColor = _SrcTex.Sample(_SrcSampler, i.uv);
	// warning X3571 : pow(f, e) will not work for negative f, use abs(f) or conditionally handle negative values if you expect them
	#pragma warning(disable:3571)
	float4 srgbColor = float4(pow(linearColor.rgb, 1.0f/2.2f), linearColor.a);
	return srgbColor;
}