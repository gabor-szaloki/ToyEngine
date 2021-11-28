#pragma warning(disable:3568) // warning X3568 : 'multi_compile' : unknown pragma ignored
#pragma multi_compile SOFT_SHADOWS_OFF SOFT_SHADOWS_TENT SOFT_SHADOWS_VARIANCE SOFT_SHADOWS_POISSON

//--------------------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------------------

#include "Surface.hlsl"
#include "Lighting.hlsl"
#include "Shadow.hlsl"

//--------------------------------------------------------------------------------------
// Input/Output structures
//--------------------------------------------------------------------------------------

struct VSOutputStandardForward
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float4 shadowCoords : TEXCOORD2;
};

//--------------------------------------------------------------------------------------
// Shader functions
//--------------------------------------------------------------------------------------

float4 get_horizontal_quad_vertex_pos(uint vertex_id)
{
	switch (vertex_id)
	{
	case 0: return float4(-0.5, 0, +0.5, 1);
	case 1: return float4(+0.5, 0, -0.5, 1);
	case 2: return float4(-0.5, 0, -0.5, 1);
	case 3: return float4(+0.5, 0, +0.5, 1);
	case 4: return float4(+0.5, 0, -0.5, 1);
	case 5: return float4(-0.5, 0, +0.5, 1);
	default: return 0;
	}
}

VSOutputStandardForward WaterVS(uint vertex_id : SV_VertexID)
{
	VSOutputStandardForward o;

	float4 objPos = get_horizontal_quad_vertex_pos(vertex_id);

	float4 worldPos = mul(_World, objPos);
	o.position = mul(_ViewProjection, worldPos);

	o.normal = normalize(mul(_World, float4(0, 1, 0, 0)).xyz);
	o.uv = worldPos.xz;
	o.worldPos = worldPos.xyz;
	o.shadowCoords = mul(_MainLightShadowMatrix, worldPos);

	return o;
}

SurfaceOutput WaterSurface(float3 pointToEye, float3 normal, float2 uv)
{
	SurfaceOutput s;

	s.albedo = float3(0.25, 0.25, 0.5);
	s.opacity = 1;
	s.normal = normal;
	s.metalness = 0;
	s.roughness = 0.1;

	return s;
}

float4 WaterPS(VSOutputStandardForward i) : SV_TARGET
{
	float4 c = 1;
	float2 screenUv = i.position.xy * _ViewportResolution.zw;

	float3 pointToEye = _CameraWorldPosition.xyz - i.worldPos;
	SurfaceOutput s = WaterSurface(pointToEye, i.normal, i.uv);

	float mainLightShadowAttenuation = SampleMainLightShadow(i.shadowCoords, screenUv);
	float ssao = 1;
	c.rgb = Lighting(s, pointToEye, mainLightShadowAttenuation, ssao);

	return c;
}