#include "Input.hlsl"

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

struct VSOutput
{
	float4 position : SV_POSITION;
	float3 viewVec : TEXCOORD0;
};

float4 get_fullscreen_triangle_vertex_pos(uint vertex_id, float z)
{
	return float4((vertex_id == 1) ? +3.0 : -1.0, (vertex_id == 2) ? -3.0 : 1.0, z, 1.0);
}

float2 get_uv_from_out_pos(float2 pos)
{
	return pos * float2(0.5, -0.5) + 0.5;
}

float3 get_view_vec(float2 uv)
{
	return lerp(lerp(_ViewVecLT, _ViewVecRT, uv.x), lerp(_ViewVecLB, _ViewVecRB, uv.x), uv.y).xyz;
}

VSOutput SkyVS(uint vertex_id : SV_VertexID)
{
	VSOutput o;

	o.position = get_fullscreen_triangle_vertex_pos(vertex_id, 1);
	float2 uv = get_uv_from_out_pos(o.position.xy);
	o.viewVec = get_view_vec(uv);

	return o;
}

// warning X3571 : pow(f, e) will not work for negative f, use abs(f) or conditionally handle negative values if you expect them
#pragma warning(disable:3571)

float3 get_sky_color(float3 view_vec)
{
	float p = view_vec.y;
	float p1 = 1 - pow(min(1, 1 - p), _TopExponent);
	float p3 = 1 - pow(min(1, 1 + p), _BottoomExponent);
	float p2 = 1 - p1 - p3;

	float3 cSky = _TopColor * p1 + _HorizonColor.rgb * p2 + _BottomColor * p3;
	float3 cSun = _MainLightColor.rgb * min(pow(max(0, dot(view_vec, -_MainLightDirection.xyz)), _SunAlpha) * _SunBeta, 1);
	return cSky * _SkyIntensity + cSun * _SunIntensity;
}

float4 SkyPS(VSOutput i) : SV_TARGET
{
	float3 v = normalize(i.viewVec);
	return float4(get_sky_color(v), 1);
}