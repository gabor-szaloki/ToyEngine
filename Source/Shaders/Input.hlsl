#ifndef INPUT_INCLUDED
#define INPUT_INCLUDED

//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------

cbuffer PerFrameConstantBuffer : register(b0)
{
	float4 _AmbientLightColor;
	float4 _MainLightColor;
	float4 _MainLightDirection;
	float4x4 _MainLightShadowMatrix;
	float4 _MainLightShadowResolution;
}

cbuffer PerCameraConstantBuffer : register(b1)
{
	float4x4 _View;
	float4x4 _Projection;
	float3 _CameraWorldPosition;
}

cbuffer PerObjectConstantBuffer : register(b2)
{
	float4x4 _World;
}

//--------------------------------------------------------------------------------------
// Textures and samplers
//--------------------------------------------------------------------------------------

Texture2D _BaseTexture : register(t0);
Texture2D _NormalTexture : register(t1);
SamplerState _Sampler : register(s0);

Texture2D<float> _MainLightShadowmap : register(t2);
SamplerComparisonState _ShadowCmpSampler : register(s2);

#endif