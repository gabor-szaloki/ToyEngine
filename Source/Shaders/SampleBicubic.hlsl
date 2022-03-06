#ifndef SAMPLE_BICUBIC_INCLUDED
#define SAMPLE_BICUBIC_INCLUDED

// Source: https://vec3.ca/bicubic-filtering-in-fewer-taps/

float4 SampleBicubicLevelZero(Texture2D tex, SamplerState linear_sampler, float2 uv, float2 tex_size, float2 inv_tex_size)
{
	float2 iTc = uv * tex_size;

	//round tc *down* to the nearest *texel center*

	float2 tc = floor(iTc - 0.5) + 0.5;

	//compute the fractional offset from that texel center
	//to the actual coordinate we want to filter at

	float2 f = iTc - tc;

	//we'll need the second and third powers
	//of f to compute our filter weights

	float2 f2 = f * f;
	float2 f3 = f2 * f;

	//compute the B-Spline weights

	float2 w0 = f2 - 0.5 * (f3 + f);
	float2 w1 = 1.5 * f3 - 2.5 * f2 + 1.0;
	float2 w3 = 0.5 * (f3 - f2);
	float2 w2 = 1.0 - w0 - w1 - w3;

	//get our texture coordinates

	float2 s0 = w0 + w1;
	float2 s1 = w2 + w3;

	float2 f0 = w1 / (w0 + w1);
	float2 f1 = w3 / (w2 + w3);

	float2 t0 = tc - 1 + f0;
	float2 t1 = tc + 1 + f1;

	//convert them to normalized coordinates

	t0 *= inv_tex_size;
	t1 *= inv_tex_size;

	//and sample and blend

	return
		(tex.SampleLevel(linear_sampler, float2(t0.x, t0.y), 0) * s0.x
	  +  tex.SampleLevel(linear_sampler, float2(t1.x, t0.y), 0) * s1.x) * s0.y
	  + (tex.SampleLevel(linear_sampler, float2(t0.x, t1.y), 0) * s0.x
	  +  tex.SampleLevel(linear_sampler, float2(t1.x, t1.y), 0) * s1.x) * s1.y;
}

float4 GetCubicWeights(float t, float a)
{
	float t2 = t*t;
	float t3 = t2 * t;
	
	float b0 = -0.5*a*t + a*t2 - 0.5*a*t3;
	float b1 = 1.0 + (0.5*a - 3.0)*t2 + (2.0 - 0.5*a)*t3;
	float b2 = 0.5*a*t + (3.0 - 1.0*a)*t2 - (2.0 - 0.5*a)*t3;
	float b3 = -0.5*a*t2 + 0.5*a*t3;
	
	return float4(b0, b1, b2, b3);
}

// Sharpening-capable bicubic sampling from Daniel Isheden

float4 SampleBicubicSharpenedLevelZero(Texture2D tex, float2 uv, float2 tex_size, float sharpening)
{
	float2 filterPos = uv * tex_size - 0.5;
	float2 floorPos = floor(filterPos);
	int2 intPos = (int2)floorPos;
	float2 t = filterPos - floorPos;

	float4 result = 0;

	float a = 1.0 + sharpening;
	float4 cubicX = GetCubicWeights(t.x, a);
	float4 cubicY = GetCubicWeights(t.y, a);

	[unroll] for (int y = 0; y < 4; y++)
	{
		[unroll] for (int x = 0; x < 4; x++)
		{
			int2 loadCoords = intPos + int2(x, y) - 1;
			float4 c = tex[loadCoords];
			result += cubicX[x] * cubicY[y] * c;
		}
	}

	return max(result, 0);
}

#endif