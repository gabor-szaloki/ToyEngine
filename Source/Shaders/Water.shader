#pragma warning(disable:3568) // warning X3568 : 'multi_compile' : unknown pragma ignored
#pragma multi_compile SOFT_SHADOWS_OFF SOFT_SHADOWS_TENT SOFT_SHADOWS_VARIANCE SOFT_SHADOWS_POISSON

//--------------------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------------------

#include "Surface.hlsl"
#include "Lighting.hlsl"
#include "Shadow.hlsl"
#include "PostFxCommon.hlsl"

//--------------------------------------------------------------------------------------
// Resources
//--------------------------------------------------------------------------------------

Texture2D<float4> _NormalMap        : register(t0);
SamplerState _NormalSampler         : register(s0);
Texture2D<float4> _SceneGrabTexture : register(t1);
SamplerState _LinearClampSampler	: register(s1);
Texture2D<float> _DepthCopyTexture	: register(t3); // t2 and s2 is used by shadowmap and its sampler -.-
SamplerState _PointClampSampler		: register(s3);

cbuffer WaterConstantBuffer : register(b4)
{
	float4x4 _WaterWorldTransform;

	float3 _FogColor;
	float _FogDensity;

	float _Roughness;
	float3 _Pad;

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
	float4 normalUvs : TEXCOORD0;
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

	o.normalUvs.xy = (worldPos.xz + _NormalAnimSpeed1 * _TimeParams.x) * _NormalTiling1;
	o.normalUvs.zw = (worldPos.xz + _NormalAnimSpeed2 * _TimeParams.x) * _NormalTiling2;

	o.worldPos = worldPos.xyz;
	o.shadowCoords = mul(_MainLightShadowMatrix, worldPos);

	return o;
}

struct WaterSurfaceOutput
{
	float3 fogColor;
	float3 refractionColor;
	float waterDepth;
	float3 normal;
	float roughness;
};

WaterSurfaceOutput WaterSurface(float3 pointToEye, float3 normal, float4 normalUvs, float2 screenUv)
{
	WaterSurfaceOutput s;

	s.fogColor = _FogColor;
	s.refractionColor = _SceneGrabTexture.Sample(_LinearClampSampler, screenUv).rgb;

	float depth = linearize_depth(_DepthCopyTexture.Sample(_PointClampSampler, screenUv));
	float3 eyeToBottom = lerp_view_vec(screenUv) * depth;
	float3 eyeToSurface = -pointToEye;
	s.waterDepth = length(eyeToBottom - eyeToSurface);

	const float2 flatTangentSpaceNormal = 0.5;
	float2 normalMapSample1 = lerp(flatTangentSpaceNormal, _NormalMap.Sample(_NormalSampler, normalUvs.xy).rg, _NormalStrength1);
	float2 normalMapSample2 = lerp(flatTangentSpaceNormal, _NormalMap.Sample(_NormalSampler, normalUvs.zw).rg, _NormalStrength2);
	s.normal = normal;
	s.normal = perturb_normal(s.normal, float3(normalMapSample1, 0), pointToEye, normalUvs.xy);
	s.normal = perturb_normal(s.normal, float3(normalMapSample2, 0), pointToEye, normalUvs.zw);

	s.roughness = _Roughness;

	return s;
}

float GetFogStrength(float waterDepth)
{
	return 1 - rcp(exp(waterDepth * _FogDensity));
}

float3 WaterLighting(WaterSurfaceOutput s, float3 pointToEye, float mainLightShadowAttenuation, float ao)
{
	float3 lightVector = -_MainLightDirection.xyz;
	float3 viewVector = normalize(pointToEye);

	const float3 F0 = 0.04; // Color at normal incidence
	float perceptualRoughness = s.roughness;
	float roughness = perceptualRoughness * perceptualRoughness; // calculate roughness to be used in PBR from perceptual roughness stored in surface

	float3 directDiffuse;
	float3 directSpecular;
	{
		// Direct light specular on water surface
		float3 kS = 0;
		float3 specular = GGX_Specular(s.normal, lightVector, viewVector, roughness, F0, kS);

		// Diffuse lit water fog
		float3 kD = 1 - kS;
		float3 lambert = s.fogColor / PI;
		float3 diffuse = kD * lambert;

		float nDotL = saturate(dot(s.normal, lightVector));
		directDiffuse = diffuse * nDotL * _MainLightColor.rgb * mainLightShadowAttenuation;
		directSpecular = specular * nDotL * _MainLightColor.rgb * mainLightShadowAttenuation;
	}

	float3 indirectDiffuse;
	float3 indirectSpecular;
	{
		float nDotV = max(dot(s.normal, viewVector), 0.0);
		float3 F = FresnelSchlickRoughness(nDotV, F0, roughness);
		float3 kS = F;

		// Specular contribution
		// Note: We adress prebaked textures with perceptualRoughness, becuase input roughness is also treated as such during the baking process.
		//       Using roughness here would mean that we square preceptualRoughness twice.
		float3 reflectionVector = reflect(-viewVector, s.normal);
		const float MAX_REFLECTION_LOD = 4.0;
		float3 prefilteredColor = _SpecularMap.SampleLevel(_LinearSampler, reflectionVector, perceptualRoughness * MAX_REFLECTION_LOD).rgb;
		float2 envBRDF = _BrdfLut.Sample(_LinearSampler, float2(nDotV, perceptualRoughness)).rg;
		float3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

		// Diffuse contribution
		float3 kD = 1 - kS;
		float3 irradiance = _IrradianceMap.Sample(_LinearSampler, s.normal).rgb;
		float3 diffuse = kD * irradiance * s.fogColor;

		indirectDiffuse = diffuse * ao;
		indirectSpecular = specular * ao;
	}

	float3 diffuseLighting = lerp(s.refractionColor, directDiffuse + indirectDiffuse, GetFogStrength(s.waterDepth));
	float3 specularLighting = directSpecular + indirectSpecular;

	return diffuseLighting + specularLighting;
}

float4 WaterPS(VSOutputStandardForward i) : SV_TARGET
{
	float4 c = 1;
	float2 screenUv = i.position.xy * _ViewportResolution.zw;

	float3 pointToEye = _CameraWorldPosition.xyz - i.worldPos;
	WaterSurfaceOutput s = WaterSurface(pointToEye, i.normal, i.normalUvs, screenUv);

	float mainLightShadowAttenuation = SampleMainLightShadow(i.shadowCoords, screenUv);
	float ssao = 1;
	c.rgb = WaterLighting(s, pointToEye, mainLightShadowAttenuation, ssao);

	//c.rgb = s.waterDepth * 0.5;

	return c;
}