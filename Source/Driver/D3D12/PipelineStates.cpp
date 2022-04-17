#include "PipelineStates.h"

#include "3rdParty/smhasher/MurmurHash3.h"

namespace drv_d3d12
{

uint64 GraphicsPipelineStateStream::hash() const
{
	uint64 result[2];
	MurmurHash3_x64_128(this, static_cast<int>(sizeof(GraphicsPipelineStateStream)), 0, result);
	return result[0];
}

}