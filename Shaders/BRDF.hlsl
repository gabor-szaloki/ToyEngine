static const float PI = 3.14159265359;

float NormalDistributionGGXTR(float3 normalVec, float3 halfwayVec, float roughness)
{
	float a = roughness;
	float a2 = a * a;   // a2 = a^2
	float NdotH = max(dot(normalVec, halfwayVec), 0.0);     // NdotH = normalVec.halfwayVec
	float NdotH2 = NdotH * NdotH;   // NdotH2 = NdotH^2
	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
	denom = PI * denom * denom;

	return nom / denom;
}


float GeometrySchlickGGX(float NdotV, float roughness)  // k is a remapping of roughness based on direct lighting or IBL lighting
{
	float r = roughness + 1.0f;
	float k = (r * r) / 8.0f;

	float nom = NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return nom / denom;
}


float GeometrySmith(float3 normalVec, float3 viewDir, float3 lightDir, float k)
{
	float NdotV = max(dot(normalVec, viewDir), 0.0f);
	float NdotL = max(dot(normalVec, lightDir), 0.0f);
	float ggx1 = GeometrySchlickGGX(NdotV, k);
	float ggx2 = GeometrySchlickGGX(NdotL, k);

	return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)   // cosTheta is n.v and F0 is the base reflectivity
{
	return (F0 + (1.0f - F0) * pow(1.0 - cosTheta, 5.0f));
}

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)   // cosTheta is n.v and F0 is the base reflectivity
{
	return F0 + (max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0f);
}

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