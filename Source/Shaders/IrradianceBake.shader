#define POSTFX_NEED_VIEW_VEC 1

#include "Common.hlsl"
#include "PostFxCommon.hlsl"

TextureCube _EnvironmentCubeMap : register(t0);
SamplerState _LinearSampler : register(s0);

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

			irradiance += _EnvironmentCubeMap.Sample(_LinearSampler, sampleVec).rgb * cosTheta * sinTheta;
			nrSamples++;
		}
	}
	irradiance = PI * irradiance * (1.0 / float(nrSamples));

	return float4(irradiance, 1);
}