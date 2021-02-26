#define POSTFX_NEED_UV 1
#define POSTFX_NEED_VIEW_VEC 1

#include "Common.hlsl"
#include "PostFxCommon.hlsl"
#include "HBAO.hlsli"

Texture2D<float> _DepthTex     : register(t0);
SamplerState _DepthSampler     : register(s0);
Texture2D<float> _RandomTex    : register(t1);

cbuffer HbaoConstantBuffer : register(b3)
{
	float4 _RadiusToScreen_R2_NegInvR2_NDotVBias;
	float4 _PowExponent_AOMultiplier_BlurSharpness_;
	float4 _Resolution_InvResolution;
	float4 _ProjInfo;
}

#define _RadiusToScreen _RadiusToScreen_R2_NegInvR2_NDotVBias.x
#define _R2 _RadiusToScreen_R2_NegInvR2_NDotVBias.y
#define _NegInvR2 _RadiusToScreen_R2_NegInvR2_NDotVBias.z
#define _NDotVBias _RadiusToScreen_R2_NegInvR2_NDotVBias.w
#define _Resolution _Resolution_InvResolution.xy
#define _PowExponent _PowExponent_AOMultiplier_BlurSharpness_.x
#define _AOMultiplier _PowExponent_AOMultiplier_BlurSharpness_.y
#define _BlurSharpness _PowExponent_AOMultiplier_BlurSharpness_.z
#define _InvResolution _Resolution_InvResolution.zw

static const uint NUM_STEPS = 4;

float3 UVToView(float2 uv, float eye_z)
{
	return float3((uv * _ProjInfo.xy + _ProjInfo.zw) * eye_z, eye_z);
}

float3 FetchViewPos(float2 uv)
{
	float ViewDepth = linearize_depth(_DepthTex.Sample(_DepthSampler, uv));
	return UVToView(uv, ViewDepth);
}

float3 MinDiff(float3 P, float3 Pr, float3 Pl)
{
	float3 V1 = Pr - P;
	float3 V2 = P - Pl;
	return (dot(V1, V1) < dot(V2, V2)) ? V1 : V2;
}

float3 ReconstructNormal(float2 uv, float3 P)
{
	float3 Pr = FetchViewPos(uv + float2(+_InvResolution.x, 0));
	float3 Pl = FetchViewPos(uv + float2(-_InvResolution.x, 0));
	float3 Pt = FetchViewPos(uv + float2(0, +_InvResolution.y));
	float3 Pb = FetchViewPos(uv + float2(0, -_InvResolution.y));
	return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));
}

float Falloff(float DistanceSquare)
{
	// 1 scalar mad instruction
	return DistanceSquare * _NegInvR2 + 1.0;
}

//----------------------------------------------------------------------------------
// P = view-space position at the kernel center
// N = view-space normal at the kernel center
// S = view-space position of the current sample
//----------------------------------------------------------------------------------
float ComputeAO(float3 P, float3 N, float3 S)
{
	float3 V = S - P;
	float VdotV = dot(V, V);
	float NdotV = dot(N, V) * 1.0 / sqrt(VdotV);

	return saturate(NdotV - _NDotVBias) * saturate(Falloff(VdotV));
}

float2 RotateDirection(float2 Dir, float2 CosSin)
{
	return float2(Dir.x * CosSin.x - Dir.y * CosSin.y,
		Dir.x * CosSin.y + Dir.y * CosSin.x);
}

float ComputeCoarseAO(float2 FullResUV, float RadiusPixels, float4 Rand, float3 ViewPosition, float3 ViewNormal)
{
	// Divide by NUM_STEPS+1 so that the farthest samples are not fully attenuated
	float StepSizePixels = RadiusPixels / (NUM_STEPS + 1);

	const float Alpha = 2.0 * PI / NUM_DIRECTIONS;
	float AO = 0;

	for (uint DirectionIndex = 0; DirectionIndex < NUM_DIRECTIONS; ++DirectionIndex)
	{
		float Angle = Alpha * DirectionIndex;

		// Compute normalized 2D direction
		float2 Direction = RotateDirection(float2(cos(Angle), sin(Angle)), Rand.xy);

		// Jitter starting sample within the first step
		float RayPixels = (Rand.z * StepSizePixels + 1.0);

		for (uint StepIndex = 0; StepIndex < NUM_STEPS; ++StepIndex)
		{
			float2 SnappedUV = round(RayPixels * Direction) * _InvResolution + FullResUV;
			float3 S = FetchViewPos(SnappedUV);

			RayPixels += StepSizePixels;

			AO += ComputeAO(ViewPosition, ViewNormal, S);
		}
	}

	AO *= _AOMultiplier / (NUM_DIRECTIONS * NUM_STEPS);
	return clamp(1.0 - AO * 2.0, 0, 1);
}

float HbaoCalcPs(DefaultPostFxVsOutput i) : SV_TARGET
{
	float3 ViewPosition = FetchViewPos(i.uv);

	// Reconstruct view-space normal from nearest neighbors
	float3 ViewNormal = -ReconstructNormal(i.uv, ViewPosition);

	// Compute projection of disk of radius _R into screen space
	float RadiusPixels = _RadiusToScreen / ViewPosition.z;

	uint2 tc = i.uv * _Resolution - 0.5;
	float4 Rand = _RandomTex[tc % RANDOM_TEX_SIZE];

	float AO = ComputeCoarseAO(i.uv, RadiusPixels, Rand, ViewPosition, ViewNormal);
	AO = pow(AO, _PowExponent);

	return AO;
}


//----------------------------------------------------------------------------------
// Bilateral blur pass
//----------------------------------------------------------------------------------

Texture2D<float> _BlurSourceTex : register(t2);
SamplerState _BlurSourceSampler : register(s2);

float BlurFunction(float2 uv, float r, float4 center_c, float center_d, inout float w_total)
{
	float c = _BlurSourceTex.Sample(_BlurSourceSampler, uv);
	float d = linearize_depth(_DepthTex.Sample(_DepthSampler, uv));

	const float BlurSigma = BLUR_KERNEL_RADIUS * 0.5;
	const float BlurFalloff = 1.0 / (2.0 * BlurSigma * BlurSigma);

	float ddiff = (d - center_d) * _BlurSharpness;
	float w = exp2(-r * r * BlurFalloff - ddiff * ddiff);
	w_total += w;

	return c * w;
}

float HbaoBlurPs(DefaultPostFxVsOutput i) : SV_TARGET
{
	float center_c = _BlurSourceTex.Sample(_BlurSourceSampler, i.uv);
	float center_d = linearize_depth(_DepthTex.Sample(_DepthSampler, i.uv));

	float c_total = center_c;
	float w_total = 1.0;

	// _InvResolution is set to:
	// (1/width,  0) - for horizontal pass
	// (0, 1/height) - for vertical pass

	uint r;
	for (r = 1; r <= uint(BLUR_KERNEL_RADIUS); ++r)
	{
		float2 uv = i.uv + _InvResolution * r;
		c_total += BlurFunction(uv, r, center_c, center_d, w_total);
	}

	for (r = 1; r <= uint(BLUR_KERNEL_RADIUS); ++r)
	{
		float2 uv = i.uv - _InvResolution * r;
		c_total += BlurFunction(uv, r, center_c, center_d, w_total);
	}

	return c_total / w_total;
}