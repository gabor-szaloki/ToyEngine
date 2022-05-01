#include "PipelineStates.h"

#include "3rdParty/smhasher/MurmurHash3.h"

namespace drv_d3d12
{

template <class T>
static void hash_combine(uint64& seed, const T& member)
{
	uint64 hash[2];
	MurmurHash3_x64_128(&member, static_cast<int>(sizeof(T)), 0, hash);
	seed ^= hash[0] + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

uint64 GraphicsPipelineStateStream::hash() const
{
	// TODO: Fix hashing as now it seems to be botched by padding in inner structs of CD3DX12_PIPELINE_STATE_STREAM_* classes

	uint64 result[2];
	MurmurHash3_x64_128(this, static_cast<int>(sizeof(GraphicsPipelineStateStream)), 0, result);
	return result[0];
	/*
	uint64 result = 0;

	hash_combine(result, rootSignature);
	hash_combine(result, inputLayout);
	hash_combine(result, primitiveTopologyType);
	hash_combine(result, vs);
	hash_combine(result, gs);
	hash_combine(result, hs);
	hash_combine(result, ds);
	hash_combine(result, ps);
	hash_combine(result, blendState);
	hash_combine(result, depthStencilState);
	hash_combine(result, depthStencilFormat);
	hash_combine(result, rasterizerState);
	hash_combine(result, renderTargetFormats);

	return result;
	*/
}

}