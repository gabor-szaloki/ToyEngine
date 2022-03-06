#include "ConstantBuffers.hlsl"
#include "PostFxCommon.hlsl"

#define BICUBIC_HISTORY 1
#if BICUBIC_HISTORY
	#include "SampleBicubic.hlsl"
#endif

Texture2D _CurrentFrameTex : register(t0);
Texture2D _HistoryTex : register(t1);
Texture2D<float> _DepthTex : register(t2);
RWTexture2D<float4> _DestTex : register(u0);
SamplerState _LinearClamp : register(s0);

cbuffer TaaConstantBuffer : register(b4)
{
	float _HistoryWeight;
	float _NeighborhoodClippingStrength;
	float _Sharpening;
	float pad;
	float4x4 _PrevViewProjMatrixWithCurrentFrameJitter;
};

float4 SampleHistory(float2 uv)
{
#if BICUBIC_HISTORY
	return SampleBicubicSharpenedLevelZero(_HistoryTex, uv, _ViewportResolution.xy, _Sharpening);
#else
	return _HistoryTex.SampleLevel(_LinearClamp, uv, 0);
#endif
}

float HistoryClip(float3 history_color, float3 current_color, float3 neighborhood_min, float3 neighborhood_max)
{
	float3 rayOrigin = history_color;
	float3 rayDir = current_color - history_color;
	rayDir = abs(rayDir) < (1.0 / 65536.0) ? (1.0 / 65536.0) : rayDir;
	float3 invRayDir = rcp(rayDir);

	float3 minIntersect = (neighborhood_min - rayOrigin) * invRayDir;
	float3 maxIntersect = (neighborhood_max - rayOrigin) * invRayDir;
	float3 enterIntersect = min(minIntersect, maxIntersect);

	return saturate(max(enterIntersect.x, max(enterIntersect.y, enterIntersect.z)) * _NeighborhoodClippingStrength);
}

[numthreads(16, 16, 1)]
void TAA(uint2 id : SV_DispatchThreadID)
{
	float depth = linearize_depth(_DepthTex[id]);
	float2 uv = (id + 0.5) * _ViewportResolution.zw;

	float3 viewVec = lerp_view_vec(uv);
	float3 worldPos = _CameraWorldPosition.xyz + viewVec * depth;
	float4 prevClipPos = mul(_PrevViewProjMatrixWithCurrentFrameJitter, float4(worldPos, 1));
	float2 historyNdc = prevClipPos.xy / prevClipPos.w;
	float2 historyUv = historyNdc * float2(0.5, -0.5) + 0.5;

	float historyWeight = _HistoryWeight;
	[flatten] if (any(abs(historyNdc) > 1))
		historyWeight = 0;

	float4 currentFrameColor = _CurrentFrameTex[id];
	float4 historyColor = SampleHistory(historyUv);

	float4 neighborhoodMin = currentFrameColor;
	float4 neighborhoodMax = currentFrameColor;
	int2 frameBounds = _ViewportResolution.xy;
	[unroll] for (int y = -1; y <= 1; y++)
	{
		[unroll] for (int x = -1; x <= 1; x++)
		{	
			if (x == 0 && y == 0)
				continue;
			int2 neighborCoord = clamp(id + int2(x, y), 0, frameBounds);
			float4 neighborColor = _CurrentFrameTex[neighborCoord];
			neighborhoodMin = min(neighborhoodMin, neighborColor);
			neighborhoodMax = max(neighborhoodMax, neighborColor);
		}
	}

	float historyClipFactor = HistoryClip(historyColor.rgb, currentFrameColor.rgb, neighborhoodMin.rgb, neighborhoodMax.rgb);
	historyColor = lerp(historyColor, currentFrameColor, historyClipFactor);

	_DestTex[id] = lerp(currentFrameColor, historyColor, historyWeight);
}