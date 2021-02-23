#include "ConstantBuffers.hlsl"

float4 ErrorVS(float4 pos : POSITION) : SV_POSITION
{
	float4 worldPos = mul(_World, pos);
	return mul(_ViewProjection, worldPos);
}

float4 ErrorPS() : SV_TARGET
{
	return float4(1, 0, 1, 1); // magenta
}