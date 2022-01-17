#include "ConstantBuffers.hlsl"
#include "PostFxCommon.hlsl"

Texture2D _CurrentFrameTex : register(t0);
Texture2D _HistoryTex : register(t1);
Texture2D<float> _DepthTex : register(t2);
RWTexture2D<float4> _DestTex : register(u0);
SamplerState _LinearClamp : register(s0);

cbuffer SkyConstantBuffer : register(b4)
{
	float _HistoryWeight;
	float3 pad;
	float4x4 _PrevViewProjMatrixWithCurrentFrameJitter;
};

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
	float4 historyColor = _HistoryTex.SampleLevel(_LinearClamp, historyUv, 0);
	_DestTex[id] = lerp(currentFrameColor, historyColor, historyWeight);
}