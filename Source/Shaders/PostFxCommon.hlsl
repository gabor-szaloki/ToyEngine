#ifndef POSTFX_COMMON_INCLUDED
#define POSTFX_COMMON_INCLUDED

#include "ConstantBuffers.hlsl"

// Full-screen triangle:
// 0---------1
// |_____|
// |
// 2

float4 get_fullscreen_triangle_vertex_pos(uint vertex_id, float z = 1.0)
{
	return float4((vertex_id == 1) ? +3.0 : -1.0, (vertex_id == 2) ? -3.0 : 1.0, z, 1.0);
}

float3 get_fullscreen_triangle_view_vec(uint vertex_id)
{
	float3 vect = (vertex_id == 0 ? _ViewVecLT : (vertex_id == 1 ? _ViewVecRT : _ViewVecLB)).xyz;
	return 2.0 * vect - _ViewVecLT.xyz;
}

float2 get_uv_from_out_pos(float2 pos)
{
	return pos * float2(0.5, -0.5) + 0.5;
}

float3 lerp_view_vec(float2 uv)
{
	return lerp(lerp(_ViewVecLT, _ViewVecRT, uv.x), lerp(_ViewVecLB, _ViewVecRB, uv.x), uv.y).xyz;
}

float linearize_depth(float raw_depth)
{
	// ze = (zNear * zFar) / (zFar - zb * (zFar - zNear)); // perspective
	return (_ProjectionParams.x / (_ProjectionParams.y * raw_depth + _ProjectionParams.z));
}

struct DefaultPostFxVsOutput
{
	float4 position : SV_POSITION;
#if POSTFX_NEED_UV
	float2 uv : TEXCOORD0;
#endif
#if POSTFX_NEED_VIEW_VEC
	float3 viewVec : TEXCOORD1;
#endif
};

DefaultPostFxVsOutput DefaultPostFxVsFunc(uint vertex_id : SV_VertexID)
{
	DefaultPostFxVsOutput o;

	o.position = get_fullscreen_triangle_vertex_pos(vertex_id);
#if POSTFX_NEED_UV
	o.uv = get_uv_from_out_pos(o.position.xy);
#endif
#if POSTFX_NEED_VIEW_VEC
	o.viewVec = get_fullscreen_triangle_view_vec(vertex_id);
#endif

	return o;
}

#endif