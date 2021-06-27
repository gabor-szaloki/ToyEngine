#ifndef RANDOM_INCLUDED
#define RANDOM_INCLUDED

// Source: https://www.shadertoy.com/view/4ssXRX

// Uniformly distributed, normalized rand, [0;1[
float nrand(float2 n)
{
	return frac(sin(dot(n.xy, float2(12.9898, 78.233))) * 43758.5453);
}

// Similar as nrand but with an additional time parameter. Works well with elapsed time in seconds
float nrand(float2 n, float t)
{
	return nrand(n + 0.07 * frac(t));
}

#endif