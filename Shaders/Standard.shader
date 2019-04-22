//--------------------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------------------

#include "Surface.hlsl"
#include "Lighting.hlsl"

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

	o.normal = normalize(mul(_World, float4(v.normal, 0)).xyz);
	o.tangent = normalize(mul(_World, float4(v.tangent.xyz, 0)).xyz);
	o.binormal = cross(o.tangent, o.normal) * v.tangent.w;

	o.color = v.color;

	o.uv = v.uv;

	o.worldPos = worldPos.xyz;

	return o;
}

float4 StandardOpaquePS(VSOutputStandard i) : SV_TARGET
{
	float4 c = 1;

	SurfaceOutput s = Surface(i.normal, i.tangent, i.binormal, i.uv, i.color);

	c.rgb = Lighting(s, i.worldPos);
	
	return c;
}