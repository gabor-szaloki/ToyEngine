#ifndef SURFACE_INCLUDED
#define SURFACE_INCLUDED

#include "Input.hlsl"
#include "NormalMapping.hlsl"

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