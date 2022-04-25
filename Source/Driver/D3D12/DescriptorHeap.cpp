#include "DescriptorHeap.h"

namespace drv_d3d12
{

DescriptorHeap::DescriptorHeap(ID3D12Device2* device_, D3D12_DESCRIPTOR_HEAP_TYPE heap_type, uint num_max_descriptors) :
	device(device_),
	heapType(heap_type),
	numMaxDescriptors(num_max_descriptors),
	descriptorSize(device_->GetDescriptorHandleIncrementSize(heap_type))
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = num_max_descriptors;
	desc.Type = heap_type;
	if (desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
		desc.Flags |= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	verify(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
}

DescriptorHeap::~DescriptorHeap()
{
	safe_release(heap);
}

DescriptorHandlePair DescriptorHeap::allocateDescriptor()
{
	std::scoped_lock<std::mutex> lock(mutex);

	assert(numDescriptors < numMaxDescriptors);
	DescriptorHandlePair handlePair
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE(heap->GetCPUDescriptorHandleForHeapStart(), numDescriptors, descriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->GetGPUDescriptorHandleForHeapStart(), numDescriptors, descriptorSize)
	};
	numDescriptors++;
	return handlePair;
}

}