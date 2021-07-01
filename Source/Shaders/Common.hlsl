#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED

static const float PI = 3.14159265359;

float inverse_lerp(float a, float b, float v)
{
	return (v - a) / (b - a);
}

float remap(float i_min, float i_max, float o_min, float o_max, float v)
{
	float t = inverse_lerp(i_min, i_max, v);
	return lerp(o_min, o_max, t);
}

float remap_clamped(float i_min, float i_max, float o_min, float o_max, float v)
{
	float t = saturate(inverse_lerp(i_min, i_max, v));
	return lerp(o_min, o_max, t);
}

#endif