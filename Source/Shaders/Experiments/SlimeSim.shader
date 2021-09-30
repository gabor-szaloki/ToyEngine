RWTexture2D<float4> _SimTarget : register(u0);

[numthreads(8, 8, 1)]
void SlimeSimCS(uint2 dtid : SV_DispatchThreadID)
{
	_SimTarget[dtid] = float4(1, 0, 0, 1);
}