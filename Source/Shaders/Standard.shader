#pragma warning(disable:3568) // warning X3568 : 'multi_compile' : unknown pragma ignored
#pragma multi_compile ALPHA_TEST_OFF ALPHA_TEST_ON
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
Texture2D<float> _SsaoTex    : register(t3);
SamplerState _SsaoTexSampler : register(s3);

//--------------------------------------------------------------------------------------
// Input/Output structures
//--------------------------------------------------------------------------------------

struct VSInputStandard
{
	float4 position : POSITION;
	float3 normal : NORMAL;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct VSOutputStandardForward
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float4 shadowCoords : TEXCOORD2;
};

struct VSOutputStandardShadow
{
	float4 position : SV_POSITION;
#if ALPHA_TEST_ON
	float2 uv : TEXCOORD0;
#endif
};

//--------------------------------------------------------------------------------------
// Shader functions
//--------------------------------------------------------------------------------------

// Forward Pass

VSOutputStandardForward StandardForwardVS(VSInputStandard v)
{
	VSOutputStandardForward o;

	float4 worldPos = mul(_World, v.position);
	o.position = mul(_ViewProjection, worldPos);

	o.normal = normalize(mul(_World, float4(v.normal, 0)).xyz);
	o.color = v.color;
	o.uv = v.uv * _MaterialUvScale * _ObjectUvScale;
	o.worldPos = worldPos.xyz;
	o.shadowCoords = mul(_MainLightShadowMatrix, worldPos);

	return o;
}

float4 StandardOpaqueForwardPS(VSOutputStandardForward i) : SV_TARGET
{
	float4 c = 1;
	float2 screenUv = i.position.xy * _ViewportResolution.zw;

	float3 pointToEye = _CameraWorldPosition.xyz - i.worldPos;
	SurfaceOutput s = Surface(pointToEye, i.normal, i.uv, i.color);

	float mainLightShadowAttenuation = SampleMainLightShadow(i.shadowCoords, screenUv);
	float ssao = _SsaoTex.Sample(_SsaoTexSampler, screenUv);
	c.rgb = Lighting(s, pointToEye, mainLightShadowAttenuation, ssao);

	return c;
}

// Shadow Pass

VSOutputStandardShadow StandardDepthOnlyVS(VSInputStandard v)
{
	VSOutputStandardShadow o;

	float4 worldPos = mul(_World, v.position);
	o.position = mul(_ViewProjection, worldPos);

#if ALPHA_TEST_ON
	o.uv = v.uv;
#endif

	return o;
}

void StandardOpaqueDepthOnlyPS(VSOutputStandardShadow i)
{
#if ALPHA_TEST_ON
	float opacity = _BaseTexture.Sample(_Sampler, i.uv).a;
	clip(opacity - ALPHA_TEST_THRESHOLD);
#endif
}