#ifndef INPUT_INCLUDED
#define INPUT_INCLUDED

//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------

cbuffer PerFrameConstantBuffer : register(b0)
{
	float4x4 _View;
	float4x4 _Projection;
	float4 _AmbientLightColor;
	float4 _MainLightColor;
	float4 _MainLightDirection;
	float3 _CameraWorldPosition;
}

cbuffer PerObjectConstantBuffer : register(b1)
{
	float4x4 _World;
}

//--------------------------------------------------------------------------------------
// Textures and samplers
//--------------------------------------------------------------------------------------

Texture2D _BaseTexture : register(t0);
Texture2D _NormalTexture : register(t1);
SamplerState _Sampler : register(s0);

#endif