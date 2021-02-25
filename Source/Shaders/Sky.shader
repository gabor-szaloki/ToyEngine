#define POSTFX_NEED_VIEW_VEC 1

#include "PostFxCommon.hlsl"

cbuffer SkyConstantBuffer : register(b3)
{
	float4 _TopColor_Exponent;
	float4 _BottomColor_Exponent;
	float4 _HorizonColor;
	float4 _SkyIntensity_SunIntensity_SunAlpha_SunBeta;
	float4 _SunAlpha_SunBeta_SunAzimuth_SunAltitude;
}

#define _TopColor _TopColor_Exponent.rgb
#define _TopExponent _TopColor_Exponent.a
#define _BottomColor _BottomColor_Exponent.rgb
#define _BottoomExponent _BottomColor_Exponent.a
#define _SkyIntensity _SkyIntensity_SunIntensity_SunAlpha_SunBeta.x
#define _SunIntensity _SkyIntensity_SunIntensity_SunAlpha_SunBeta.y
#define _SunAlpha _SkyIntensity_SunIntensity_SunAlpha_SunBeta.z
#define _SunBeta _SkyIntensity_SunIntensity_SunAlpha_SunBeta.w

// warning X3571 : pow(f, e) will not work for negative f, use abs(f) or conditionally handle negative values if you expect them
#pragma warning(disable:3571)

float3 get_sky_color(float3 view_dir)
{
	float p = view_dir.y;
	float p1 = 1 - pow(min(1, 1 - p), _TopExponent);
	float p3 = 1 - pow(min(1, 1 + p), _BottoomExponent);
	float p2 = 1 - p1 - p3;

	float3 cSky = _TopColor * p1 + _HorizonColor.rgb * p2 + _BottomColor * p3;
	float3 cSun = _MainLightColor.rgb * min(pow(max(0, dot(view_dir, -_MainLightDirection.xyz)), _SunAlpha) * _SunBeta, 1);
	return cSky * _SkyIntensity + cSun * _SunIntensity;
}

float4 SkyPS(DefaultPostFxVsOutput i) : SV_TARGET
{
	float3 viewDir = normalize(i.viewVec);
	return float4(get_sky_color(viewDir), 1);
}