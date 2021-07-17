#ifndef SURFACE_INCLUDED
#define SURFACE_INCLUDED

#include "ConstantBuffers.hlsl"
#include "NormalMapping.hlsl"

#define ALPHA_TEST_THRESHOLD 0.5

Texture2D _BaseTexture             : register(t0);
Texture2D _NormalRoughMetalTexture : register(t1);
SamplerState _Sampler              : register(s0);

struct SurfaceOutput
{
	float3 albedo;
	float opacity;
	float3 normal;
	float roughness;
	float metalness;
};

SurfaceOutput Surface(float3 pointToEye, float3 normal, float2 uv, float4 vertColor)
{
	SurfaceOutput s;

	// Albedo + Opacity
	float4 baseTextureSample = _BaseTexture.Sample(_Sampler, uv);
	s.albedo = baseTextureSample.rgb * _MaterialColor.rgb * vertColor.rgb;
	s.opacity = baseTextureSample.a * _MaterialColor.a;

	// Normal
	float4 normalRoughMetal = _NormalRoughMetalTexture.Sample(_Sampler, uv);
	s.normal = perturb_normal(normal, float3(normalRoughMetal.xy, 0), pointToEye, uv);

	// Metalness
	s.metalness = normalRoughMetal.z * _MaterialParams.x + _MaterialParams.y;

	// Roughness
	s.roughness = normalRoughMetal.w * _MaterialParams.z + _MaterialParams.w;

	return s;
}

#endif