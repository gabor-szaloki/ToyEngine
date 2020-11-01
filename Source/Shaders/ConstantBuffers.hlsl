#ifndef CONSTANT_BUFFERS_INCLUDED
#define CONSTANT_BUFFERS_INCLUDED

cbuffer PerFrameConstantBuffer : register(b0)
{
	float4 _AmbientLightColor;
	float4 _MainLightColor;
	float4 _MainLightDirection;
	float4x4 _MainLightShadowMatrix;
	float4 _MainLightShadowResolution;
}

cbuffer PerCameraConstantBuffer : register(b1)
{
	float4x4 _View;
	float4x4 _Projection;
	float4 _CameraWorldPosition;
	float4 _ViewVecLT;
	float4 _ViewVecRT;
	float4 _ViewVecLB;
	float4 _ViewVecRB;
}

cbuffer PerObjectConstantBuffer : register(b2)
{
	float4x4 _World;
}

#endif