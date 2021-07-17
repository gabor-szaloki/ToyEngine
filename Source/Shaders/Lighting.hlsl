#ifndef LIGHTING_INCLUDED
#define LIGHTING_INCLUDED

#include "BRDF.hlsl"
#include "Surface.hlsl"

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

	// Specular contribution
	float3 F0 = lerp(0.04, s.albedo, s.metalness); // Color at normal incidence
	float3 kS = 0;
	float3 specular = GGX_Specular(s.normal, lightVector, viewVector, s.roughness, F0, kS);

	// Diffuse contribution
	float3 kD = (1 - kS) * (1 - s.metalness);
	float3 lambert = s.albedo / PI;
	float3 diffuse = kD * lambert;

	float nDotL = saturate(dot(s.normal, lightVector));
	float3 directLighting = (diffuse + specular) * nDotL * _MainLightColor.rgb * mainLightShadowAttenuation;

	// Ambient contribution
	float3 ambientColor = lerp(_AmbientLightBottomColor.rgb, _AmbientLightTopColor.rgb, s.normal.y * 0.5 + 0.5);
	float3 ambient = s.albedo * ambientColor * 0.5;

	float3 indirectLighting = ambient * ao;

	return directLighting + indirectLighting;
}

#endif