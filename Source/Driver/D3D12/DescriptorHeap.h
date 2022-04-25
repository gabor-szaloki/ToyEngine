#pragma once

#include "DriverCommonD3D12.h"
#include <mutex>

namespace drv_d3d12
{
	struct DescriptorHandlePair
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	};

	class DescriptorHeap
	{
	public:
		DescriptorHeap(ID3D12Device2* device_, D3D12_DESCRIPTOR_HEAP_TYPE heap_type, uint num_max_descriptors);
		~DescriptorHeap();
		ID3D12DescriptorHeap* getHeap() const { return heap; }
		ID3D12DescriptorHeap** getHeapAddress() { return &heap; }
		DescriptorHandlePair allocateDescriptor();

	private:
		ID3D12Device2* device;
		ID3D12DescriptorHeap* heap = nullptr;
		const D3D12_DESCRIPTOR_HEAP_TYPE heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		const uint descriptorSize = 0;
		const uint numMaxDescriptors = 0;
		uint numDescriptors = 0;
		std::mutex mutex;
	};
}