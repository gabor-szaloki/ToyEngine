#ifndef SURFACE_INCLUDED
#define SURFACE_INCLUDED

#include "ConstantBuffers.hlsl"
#include "NormalMapping.hlsl"

Texture2D _BaseTexture   : register(t0);
Texture2D _NormalTexture : register(t1);
SamplerState _Sampler    : register(s0);

struct SurfaceOutput
{
	float3 albedo;
	float3 normal;
	float roughness;
	float metalness;
};

SurfaceOutput Surface(float3 pointToEye, float3 normal, float2 uv, float4 vertColor)
{
	SurfaceOutput s;

	// Albedo
	float4 baseTextureSample = _BaseTexture.Sample(_Sampler, uv);
	s.albedo = baseTextureSample.rgb * vertColor.rgb;

	// Normal
	float3 normalTextureSample = _NormalTexture.Sample(_Sampler, uv).rgb;
	s.normal = perturb_normal(normal, normalTextureSample, pointToEye, uv);

	// Roughness
	s.roughness = baseTextureSample.a;

	// Metalness
	s.metalness = 0.0f;

	return s;
}

#endif