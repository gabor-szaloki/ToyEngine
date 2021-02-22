#pragma warning(disable:3568) // warning X3568 : 'multi_compile' : unknown pragma ignored
#pragma multi_compile ALPHA_TEST_OFF ALPHA_TEST_ON

//--------------------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------------------

#include "Surface.hlsl"
#include "Lighting.hlsl"
#include "Shadow.hlsl"

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
	float4 viewPos = mul(_View, worldPos);
	float4 clipPos = mul(_Projection, viewPos);
	o.position = clipPos;

	o.normal = normalize(mul(_World, float4(v.normal, 0)).xyz);
	o.color = v.color;
	o.uv = v.uv;
	o.worldPos = worldPos.xyz;
	o.shadowCoords = mul(_MainLightShadowMatrix, worldPos);

	return o;
}

float4 StandardOpaqueForwardPS(VSOutputStandardForward i) : SV_TARGET
{
	float4 c = 1;

	float3 pointToEye = _CameraWorldPosition.xyz - i.worldPos;
	SurfaceOutput s = Surface(pointToEye, i.normal, i.uv, i.color);

	float mainLightShadowAttenuation = SampleMainLightShadow(i.shadowCoords);
	c.rgb = Lighting(s, pointToEye, mainLightShadowAttenuation);

	return c;
}

// Shadow Pass

VSOutputStandardShadow StandardDepthOnlyVS(VSInputStandard v)
{
	VSOutputStandardShadow o;

	float4 worldPos = mul(_World, v.position);
	float4 viewPos = mul(_View, worldPos);
	float4 clipPos = mul(_Projection, viewPos);
	o.position = clipPos;

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