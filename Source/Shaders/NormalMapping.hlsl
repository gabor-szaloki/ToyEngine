#ifndef NORMAL_MAPPING_INCLUDED
#define NORMAL_MAPPING_INCLUDED

// http://www.thetenthplanet.de/archives/1180

#define WITH_NORMALMAP_UNSIGNED 1
#define WITH_NORMALMAP_GREEN_UP 1
#define WITH_NORMALMAP_2CHANNEL 1

float3x3 cotangent_frame(float3 N, float3 p, float2 uv)
{
	// get edge vec­tors of the pix­el tri­an­gle
	float3 dp1 = ddx(p);
	float3 dp2 = ddy(p);
	float2 duv1 = ddx(uv);
	float2 duv2 = ddy(uv);

	// solve the lin­ear sys­tem
	float3 dp2perp = cross(dp2, N);
	float3 dp1perp = cross(N, dp1);
	float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

	// con­struct a scale-invari­ant frame 
	float invmax = rsqrt(max(dot(T, T), dot(B, B)));
	return float3x3(T * invmax, B * invmax, N);
}

float3 perturb_normal(float3 original_normal, float3 normal_map, float3 view, float2 uv)
{
#ifdef WITH_NORMALMAP_UNSIGNED
	normal_map = normal_map * 255. / 127. - 128. / 127.;
#endif
#ifdef WITH_NORMALMAP_2CHANNEL
	normal_map.z = sqrt(1. - dot(normal_map.xy, normal_map.xy));
#endif
#ifdef WITH_NORMALMAP_GREEN_UP
	normal_map.y = -normal_map.y;
#endif
	float3x3 TBN = cotangent_frame(original_normal, -view, uv);
	return normalize(mul(normal_map, TBN));
}

#endif