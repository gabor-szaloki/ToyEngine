Texture2D _CurrentFrameTex : register(t0);
Texture2D _HistoryTex : register(t1);
RWTexture2D<float4> _DestTex : register(u0);

[numthreads(16, 16, 1)]
void TAA(uint2 id : SV_DispatchThreadID)
{
	float4 currentFrameColor = _CurrentFrameTex[id];
	float4 historyColor = _HistoryTex[id];
	_DestTex[id] = lerp(historyColor, currentFrameColor, 0.05);
}