#define POSTFX_NEED_VIEW_VEC 1
#define POSTFX_NEED_UV 1

#include "Common.hlsl"
#include "PostFxCommon.hlsl"
#include "BRDF.hlsl"
#include "Hammersley.hlsl"

TextureCube _EnvironmentCubeMap : register(t0);
SamplerState _LinearSampler : register(s0);

cbuffer EnvironmentBakeConstantBuffer : register(b3)
{
	float4 _SourceCubeWidth_InvSourceCubeWidth_Roughness_RadianceCutoff;
}

#define _SourceCubeWidth _SourceCubeWidth_InvSourceCubeWidth_Roughness_RadianceCutoff.x
#define _InvSourceCubeWidth _SourceCubeWidth_InvSourceCubeWidth_Roughness_RadianceCutoff.y
#define _Roughness _SourceCubeWidth_InvSourceCubeWidth_Roughness_RadianceCutoff.z
#define _RadianceCutoff _SourceCubeWidth_InvSourceCubeWidth_Roughness_RadianceCutoff.w

float3 SampleEnvironment(float3 dir)
{
	return min(_EnvironmentCubeMap.Sample(_LinearSampler, dir).rgb, _RadianceCutoff);
}

float3 SampleEnvironment(float3 dir, float mip)
{
	return min(_EnvironmentCubeMap.SampleLevel(_LinearSampler, dir, mip).rgb, _RadianceCutoff);
}

float4 IrradianceBakePS(DefaultPostFxVsOutput i) : SV_TARGET
{
	// the sample direction equals the hemisphere's orientation 
	float3 normal = normalize(i.viewVec);

	float3 irradiance = 0;

	float3 up = float3(0.0, 1.0, 0.0);
	float3 right = normalize(cross(normal, up));
	up = normalize(cross(right, normal));

	const float sampleDelta = 0.025;
	float nrSamples = 0.0;
	for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
	{
		float sinPhi, cosPhi;
		sincos(phi, sinPhi, cosPhi);
		for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
		{
			float sinTheta, cosTheta;
			sincos(theta, sinTheta, cosTheta);

			// spherical to cartesian (in tangent space)
			float3 tangentSample = float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
			// tangent space to world
			float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

			irradiance += SampleEnvironment(sampleVec) * cosTheta * sinTheta;
			nrSamples++;
		}
	}
	irradiance = PI * irradiance * (1.0 / float(nrSamples));

	return float4(irradiance, 1);
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	float cosPhi, sinPhi;
	sincos(phi, sinPhi, cosPhi);

	// from spherical coordinates to cartesian coordinates
	float3 H;
	H.x = cosPhi * sinTheta;
	H.y = sinPhi * sinTheta;
	H.z = cosTheta;

	// from tangent-space vector to world-space sample vector
	float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangent = normalize(cross(N, up));
	float3 bitangent = cross(tangent, N);

	float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

static const uint SPECULAR_SAMPLE_COUNT = 1024u;

float calculate_source_mip(float3 N, float3 H, float3 V, float roughness, float resolution)
{
	roughness = roughness * roughness;

	float D = NormalDistributionGGXTR(N, H, roughness);
	float NdotH = max(dot(N, H), 0.0);
	float HdotV = max(dot(H, V), 0.0);
	float pdf = (D * NdotH / (4.0 * HdotV)) + 0.0001;

	float saTexel = 4.0 * PI / (6.0 * resolution * resolution);
	float saSample = 1.0 / (float(SPECULAR_SAMPLE_COUNT) * pdf + 0.0001);

	float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);
	return mipLevel;
}

float4 SpecularBakePS(DefaultPostFxVsOutput i) : SV_TARGET
{
	float3 N = normalize(i.viewVec);
	float3 R = N;
	float3 V = R;

	float totalWeight = 0.0;
	float3 prefilteredColor = 0;
	for (uint i = 0u; i < SPECULAR_SAMPLE_COUNT; ++i)
	{
		float2 Xi = Hammersley(i, SPECULAR_SAMPLE_COUNT);
		float3 H = ImportanceSampleGGX(Xi, N, _Roughness);
		float3 L = normalize(2.0 * dot(V, H) * H - V);

		float NdotL = max(dot(N, L), 0.0);
		if (NdotL > 0.0)
		{
			float mipLevel = calculate_source_mip(N, H, V, _Roughness, _SourceCubeWidth);
			prefilteredColor += SampleEnvironment(L, mipLevel).rgb * NdotL;
			totalWeight += NdotL;
		}
	}
	prefilteredColor = prefilteredColor / totalWeight;

	return float4(prefilteredColor, 1);
}

float2 IntegrateBrdfPS(DefaultPostFxVsOutput i) : SV_TARGET
{
	float NdotV = i.uv.x;
	float roughness = i.uv.y;

	float3 V;
	V.x = sqrt(1.0 - NdotV * NdotV);
	V.y = 0.0;
	V.z = NdotV;

	float A = 0.0;
	float B = 0.0;

	float3 N = float3(0.0, 0.0, 1.0);

	for (uint i = 0u; i < SPECULAR_SAMPLE_COUNT; ++i)
	{
		float2 Xi = Hammersley(i, SPECULAR_SAMPLE_COUNT);
		float3 H = ImportanceSampleGGX(Xi, N, roughness);
		float3 L = normalize(2.0 * dot(V, H) * H - V);

		float NdotL = max(L.z, 0.0);
		float NdotH = max(H.z, 0.0);
		float VdotH = max(dot(V, H), 0.0);

		if (NdotL > 0.0)
		{
			float k = KGeometryGGXIBL(roughness);
			float G = GeometrySmith(N, V, L, k);
			float G_Vis = (G * VdotH) / (NdotH * NdotV);
			float Fc = pow(1.0 - VdotH, 5.0);

			A += (1.0 - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}
	A /= float(SPECULAR_SAMPLE_COUNT);
	B /= float(SPECULAR_SAMPLE_COUNT);

	return float2(A, B);
}