#ifndef CONSTANT_BUFFERS_INCLUDED
#define CONSTANT_BUFFERS_INCLUDED

cbuffer PerFrameConstantBuffer : register(b0)
{
	float4 _MainLightColor;
	float4 _MainLightDirection;
	float4x4 _MainLightShadowMatrix;
	float4 _MainLightShadowParams; // x: resolution, y: 1/resolution, z: range, w: radius for poisson samples in shadowmap uv
	float4 _TonemappingParams; // x: exposure, y,z,w: unused
	float4 _TimeParams; // x: elapsed seconds, y,z,w: unused
}

cbuffer PerCameraConstantBuffer : register(b1)
{
	float4x4 _View;
	float4x4 _Projection;
	float4x4 _ViewProjection;
	float4 _ProjectionParams; // x: zn*zf, y: zn-zf, z: zf, w: perspective = 1 : 0
	float4 _CameraWorldPosition;
	float4 _ViewportResolution;
	float4 _ViewVecLT;
	float4 _ViewVecRT;
	float4 _ViewVecLB;
	float4 _ViewVecRB;
}

cbuffer PerObjectConstantBuffer : register(b2)
{
	float4x4 _World;
}

cbuffer PerMaterialConstantBuffer : register(b3)
{
	float4 _MaterialColor;
	float4 _MaterialParams; // x: metalness scale, y: metalness bias, z: roughness scale, w: roughness bias
}

#endif