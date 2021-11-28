#pragma warning(disable:3568) // warning X3568 : 'multi_compile' : unknown pragma ignored
#pragma multi_compile SOFT_SHADOWS_OFF SOFT_SHADOWS_TENT SOFT_SHADOWS_VARIANCE SOFT_SHADOWS_POISSON

//--------------------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------------------

#include "Surface.hlsl"
#include "Lighting.hlsl"
#include "Shadow.hlsl"

//--------------------------------------------------------------------------------------
// Resources
//--------------------------------------------------------------------------------------

Texture2D<float4> _NormalMap    : register(t0);
SamplerState _NormalSampler     : register(s0);

cbuffer WaterConstantBuffer : register(b4)
{
	float4x4 _WaterWorldTransform;

	float3 _Albedo;
	float _Roughness;

	float _NormalStrength1;
	float _NormalTiling1;
	float2 _NormalAnimSpeed1;

	float _NormalStrength2;
	float _NormalTiling2;
	float2 _NormalAnimSpeed2;
}

//--------------------------------------------------------------------------------------
// Input/Output structures
//--------------------------------------------------------------------------------------

struct VSOutputStandardForward
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float4 normalUVs : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float4 shadowCoords : TEXCOORD2;
};

//--------------------------------------------------------------------------------------
// Shader functions
//--------------------------------------------------------------------------------------

float4 get_horizontal_quad_vertex_pos(uint vertex_id)
{
	switch (vertex_id)
	{
	case 0: return float4(-0.5, 0, +0.5, 1);
	case 1: return float4(+0.5, 0, -0.5, 1);
	case 2: return float4(-0.5, 0, -0.5, 1);
	case 3: return float4(+0.5, 0, +0.5, 1);
	case 4: return float4(+0.5, 0, -0.5, 1);
	case 5: return float4(-0.5, 0, +0.5, 1);
	default: return 0;
	}
}

VSOutputStandardForward WaterVS(uint vertex_id : SV_VertexID)
{
	VSOutputStandardForward o;

	float4 objPos = get_horizontal_quad_vertex_pos(vertex_id);

	float4 worldPos = mul(_WaterWorldTransform, objPos);
	o.position = mul(_ViewProjection, worldPos);

	o.normal = normalize(mul(_World, float4(0, 1, 0, 0)).xyz);

	o.normalUVs.xy = (worldPos.xz + _NormalAnimSpeed1 * _TimeParams.x) * _NormalTiling1;
	o.normalUVs.zw = (worldPos.xz + _NormalAnimSpeed2 * _TimeParams.x) * _NormalTiling2;

	o.worldPos = worldPos.xyz;
	o.shadowCoords = mul(_MainLightShadowMatrix, worldPos);

	return o;
}

SurfaceOutput WaterSurface(float3 pointToEye, float3 normal, float4 uvs)
{
	SurfaceOutput s;

	s.albedo = _Albedo;
	s.opacity = 1;

	const float2 flatTangentSpaceNormal = 0.5;
	float2 normalMapSample1 = lerp(flatTangentSpaceNormal, _NormalMap.Sample(_NormalSampler, uvs.xy).rg, _NormalStrength1);
	float2 normalMapSample2 = lerp(flatTangentSpaceNormal, _NormalMap.Sample(_NormalSampler, uvs.zw).rg, _NormalStrength2);
	s.normal = normal;
	s.normal = perturb_normal(s.normal, float3(normalMapSample1, 0), pointToEye, uvs.xy);
	s.normal = perturb_normal(s.normal, float3(normalMapSample2, 0), pointToEye, uvs.zw);

	s.metalness = 0;
	s.roughness = _Roughness;

	return s;
}

float4 WaterPS(VSOutputStandardForward i) : SV_TARGET
{
	float4 c = 1;
	float2 screenUv = i.position.xy * _ViewportResolution.zw;

	float3 pointToEye = _CameraWorldPosition.xyz - i.worldPos;
	SurfaceOutput s = WaterSurface(pointToEye, i.normal, i.normalUVs);

	float mainLightShadowAttenuation = SampleMainLightShadow(i.shadowCoords, screenUv);
	float ssao = 1;
	c.rgb = Lighting(s, pointToEye, mainLightShadowAttenuation, ssao);

	return c;
}