#ifndef POSTFX_COMMON_INCLUDED
#define POSTFX_COMMON_INCLUDED

#include "ConstantBuffers.hlsl"

float4 get_fullscreen_triangle_vertex_pos(uint vertex_id, float z = 1.0)
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

struct DefaultPostFxVsOutput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

DefaultPostFxVsOutput DefaultPostFxVsFunc(uint vertex_id : SV_VertexID)
{
	DefaultPostFxVsOutput o;

	o.position = get_fullscreen_triangle_vertex_pos(vertex_id);
	o.uv = get_uv_from_out_pos(o.position.xy);

	return o;
}

#endif