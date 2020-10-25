#ifndef LIGHTING_INCLUDED
#define LIGHTING_INCLUDED

#include "BRDF.hlsl"
#include "Surface.hlsl"

float3 GGX_Specular(float3 lightColor, float3 normal, float3 lightVector, float3 viewVector, float perceptualRoughness, float3 F0, out float3 kS)
{
	float roughness = perceptualRoughness * perceptualRoughness;
	float3 reflectionVector = reflect(-viewVector, normal);
	float3 halfVector = normalize(lightVector + viewVector);

	// Distribution
	float D = NormalDistributionGGXTR(normal, halfVector, roughness);

	// Calculate fresnel
	float3 F = FresnelSchlick(saturate(dot(halfVector, viewVector)), F0);
	kS = F;

	// Geometry term
	float G = GeometrySmith(normal, viewVector, lightVector, roughness);

	// Calculate the Cook-Torrance denominator
	float vDotN = saturate(dot(viewVector, normal));
	float hDotN = saturate(dot(halfVector, normal));
	float denominator = saturate(4 * vDotN * hDotN + 0.05);

	float3 radiance = lightColor * D * G * F / denominator;
	return radiance;
}

float3 Lighting(SurfaceOutput s, float3 pointToEye, float mainLightShadowAttenuation)
{
	float3 lightVector = -_MainLightDirection.xyz;
	float3 viewVector = normalize(pointToEye);

	// Color at normal incidence
	float3 F0 = lerp(0.04, s.albedo, s.metalness);

	// Specular contribution
	float3 kS = 0;
	float3 specular = GGX_Specular(_MainLightColor.rgb, s.normal, lightVector, viewVector, s.roughness, F0, kS);

	// Diffuse contribution
	float3 kD = (1 - kS) * (1 - s.metalness);
	float nDotL = saturate(dot(s.normal, lightVector));
	float3 lambert = nDotL * _MainLightColor.rgb;
	float3 diffuse = s.albedo * lambert;

	// Ambient contribution
	float3 ambient = s.albedo * _AmbientLightColor.rgb;

	return ambient + (kD * diffuse +/* kS * */specular) * mainLightShadowAttenuation;
}

#endif