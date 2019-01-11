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

VSOutputStandard StandardVS(VSInputStandard v)
{
	VSOutputStandard o;

	o.position = v.position;
	o.color = v.color;

	return o;
}


float4 StandardOpaquePS(VSOutputStandard i) : SV_TARGET
{
	return i.color;
}