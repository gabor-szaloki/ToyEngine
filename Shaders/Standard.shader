//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------
cbuffer PerFrameConstantBuffer : register(b0)
{
	float4x4 _View;
	float4x4 _Projection;
}

cbuffer PerObjectConstantBuffer : register(b1)
{
	float4x4 _World;
}

//--------------------------------------------------------------------------------------
// Input/Output structures
//--------------------------------------------------------------------------------------
struct VSInputStandard
{
	float4 position : POSITION;
	float4 color : COLOR;
};

struct VSOutputStandard
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
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

	o.color = v.color;

	return o;
}


float4 StandardOpaquePS(VSOutputStandard i) : SV_TARGET
{
	return i.color;
}