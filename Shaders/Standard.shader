//--------------------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------------------
#include "BRDF.hlsl"

//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------
cbuffer PerFrameConstantBuffer : register(b0)
{
	float4x4 _View;
	float4x4 _Projection;
	float4 _AmbientLightColor;
	float4 _MainLightColor;
	float4 _MainLightDirection;
	float3 _CameraWorldPosition;
}

cbuffer PerObjectConstantBuffer : register(b1)
{
	float4x4 _World;
}

//--------------------------------------------------------------------------------------
// Textures and samplers
//--------------------------------------------------------------------------------------
Texture2D _BaseTexture : register(t0);
Texture2D _NormalTexture : register(t1);
SamplerState _Sampler : register(s0);

//--------------------------------------------------------------------------------------
// Input/Output structures
//--------------------------------------------------------------------------------------
struct VSInputStandard
{
	float4 position : POSITION;
	float3 normal : NORMAL;
	float4 tangent : TANGENT;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct VSOutputStandard
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
};

//--------------------------------------------------------------------------------------
// Shader functions
//--------------------------------------------------------------------------------------

VSOutputStandard StandardVS(VSInputStandard v)
{
	VSOutputStandard o;

	float4 worldPos = mul(_World, v.position);
	float4 viewPos = mul(_View, worldPos);
	float4 clipPos = mul(_Projection, viewPos);
	o.position = clipPos;

	o.normal = mul(_World, float4(v.normal, 0)).xyz;
	o.tangent = mul(_World, float4(v.tangent.xyz, 0)).xyz;
	o.binormal = cross(o.tangent, o.normal) * v.tangent.w;

	o.color = v.color;

	o.uv = v.uv;

	o.worldPos = worldPos.xyz;

	return o;
}


float4 StandardOpaquePS(VSOutputStandard i) : SV_TARGET
{
	float4 c = 1;

	//
	// Surface
	//

	float4 baseTextureSample = _BaseTexture.Sample(_Sampler, i.uv);
	float3 normalTextureSample = _NormalTexture.Sample(_Sampler, i.uv).rgb;
	normalTextureSample = normalTextureSample * 2.0 - 1.0;

	float3 albedo = baseTextureSample.rgb * i.color.rgb;

	float3 normal = i.tangent * normalTextureSample.x + i.binormal * normalTextureSample.y + i.normal * normalTextureSample.z;
	normal = normalize(normal);

	float roughness = baseTextureSample.a;
	float metalness = 0.0f;

	//
	// Lighting
	//
	
	float3 lightVector = -_MainLightDirection.xyz;
	float3 viewVector = normalize(_CameraWorldPosition - i.worldPos);

	// Color at normal incidence
	float3 F0 = lerp(0.04, albedo, metalness);
	
	// Specular contribution
	float3 kS = 0;
	float3 specular = GGX_Specular(_MainLightColor.rgb, normal, lightVector, viewVector, roughness, F0, kS);

	// Diffuse contribution
	float3 kD = (1 - kS) * (1 - metalness);
	float nDotL = saturate(dot(normal, lightVector));
	float3 lambert = nDotL * _MainLightColor.rgb;
	float3 diffuse = albedo * lambert;

	// Ambient contribution
	float3 ambient = albedo * _AmbientLightColor.rgb;

	c.rgb = ambient + kD * diffuse +/* kS * */specular;

	// Debug
	//c.rgb = specular;

	return c;
}