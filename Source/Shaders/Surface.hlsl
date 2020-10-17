#ifndef SURFACE_INCLUDED
#define SURFACE_INCLUDED

#include "Input.hlsl"

struct SurfaceOutput
{
	float3 albedo;
	float3 normal;
	float roughness;
	float metalness;
};

SurfaceOutput Surface(float3 normal, float3 tangent, float3 binormal, float2 uv, float4 vertColor)
{
	SurfaceOutput s;

	// Albedo
	float4 baseTextureSample = _BaseTexture.Sample(_Sampler, uv);
	s.albedo = baseTextureSample.rgb * vertColor.rgb;

	// Normal
	float3 normalTextureSample = _NormalTexture.Sample(_Sampler, uv).rgb;
	normalTextureSample = normalTextureSample * 2.0 - 1.0;
	s.normal = tangent * normalTextureSample.x + binormal * normalTextureSample.y + normal * normalTextureSample.z;
	s.normal = normalize(s.normal);

	// Roughness
	s.roughness = baseTextureSample.a;

	// Metalness
	s.metalness = 0.0f;

	return s;
}

#endif