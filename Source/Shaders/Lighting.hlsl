#ifndef LIGHTING_INCLUDED
#define LIGHTING_INCLUDED

#include "BRDF.hlsl"
#include "Surface.hlsl"

TextureCube _IrradianceMap : register(t10);
TextureCube _SpecularMap : register(t11);
Texture2D _BrdfLut : register(t12);
SamplerState _LinearSampler : register(s10);

float3 GGX_Specular(float3 normal, float3 lightVector, float3 viewVector, float perceptualRoughness, float3 F0, out float3 kS)
{
	float roughness = perceptualRoughness * perceptualRoughness;
	float3 halfVector = normalize(lightVector + viewVector);

	// Distribution
	float D = NormalDistributionGGXTR(normal, halfVector, roughness);

	// Calculate Fresnel
	// Note: instead of nDotV, we pass hDotV as cosTheta to Fresnel, because halfway vector represents the microsurface normal that reflects
	//       light in the view direction. If microsurface normal would face in any other directions, it wouldn't pass the NDF test. This is
	//       explained in detail in the following comment: http://disq.us/p/1etzl77 under this article: https://learnopengl.com/PBR/Lighting
	float cosTheta = saturate(dot(halfVector, viewVector));
	float3 F = FresnelSchlick(cosTheta, F0);
	kS = F;

	// Geometry term
	float kGeom = KGeometryGGXDirect(roughness);
	float G = GeometrySmith(normal, viewVector, lightVector, kGeom);

	// Calculate the Cook-Torrance numerator and denominator
	float3 numerator = D * G * F;
	float vDotN = max(dot(viewVector, normal), 0);
	float hDotN = max(dot(halfVector, normal), 0);
	float denominator = 4 * vDotN * hDotN;

	return numerator / max(denominator, 0.001);
}

float3 Lighting(SurfaceOutput s, float3 pointToEye, float mainLightShadowAttenuation, float ao)
{
	float3 lightVector = -_MainLightDirection.xyz;
	float3 viewVector = normalize(pointToEye);

	float3 F0 = lerp(0.04, s.albedo, s.metalness); // Color at normal incidence

	float3 directLighting;
	{
		// Specular contribution
		float3 kS = 0;
		float3 specular = GGX_Specular(s.normal, lightVector, viewVector, s.roughness, F0, kS);

		// Diffuse contribution
		float3 kD = (1 - kS) * (1 - s.metalness);
		float3 lambert = s.albedo / PI;
		float3 diffuse = kD * lambert;

		float nDotL = saturate(dot(s.normal, lightVector));
		directLighting = (diffuse + specular) * nDotL * _MainLightColor.rgb * mainLightShadowAttenuation;
	}

	float3 indirectLighting;
	{
		// Specular contribution
		float roughness = s.roughness * s.roughness; // calculate roughness to be used in PBR from perceptual roughness stored in surface
		float nDotV = max(dot(s.normal, viewVector), 0.0);
		float3 F = FresnelSchlickRoughness(nDotV, F0, roughness);
		float3 kS = F;
		float3 reflectionVector = reflect(-viewVector, s.normal);
		const float MAX_REFLECTION_LOD = 4.0;
		float3 prefilteredColor = _SpecularMap.SampleLevel(_LinearSampler, reflectionVector, roughness * MAX_REFLECTION_LOD).rgb;
		float2 envBRDF = _BrdfLut.Sample(_LinearSampler, float2(nDotV, roughness)).rg;
		float3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

		// Diffuse contribution
		float3 kD = (1 - kS) * (1 - s.metalness);
		float3 irradiance = _IrradianceMap.Sample(_LinearSampler, s.normal).rgb;
		float3 diffuse = kD * irradiance * s.albedo;

		indirectLighting = (diffuse + specular) * ao;
	}

	return directLighting + indirectLighting;
}

#endif