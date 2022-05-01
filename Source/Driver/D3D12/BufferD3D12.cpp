#include "BufferD3D12.h"

namespace drv_d3d12
{

Buffer::Buffer(const BufferDesc& desc_) : desc(desc_)
{
	// Create a committed resource for the GPU resource in a default heap.
	{
		const uint64 bufferSize = (uint64)desc.numElements * (uint64)desc.elementByteSize;
		const D3D12_RESOURCE_FLAGS flags = (desc.bindFlags & BIND_UNORDERED_ACCESS) ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
		const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
		const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
		verify(DriverD3D12::get().getDevice().CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			resourceState,
			nullptr,
			IID_PPV_ARGS(&resource)));
	}

	if (desc.initialData != nullptr)
		updateData(desc.initialData);

	set_debug_name(resource, desc.name);

	id = DriverD3D12::get().registerBuffer(this);
}

Buffer::~Buffer()
{
	DriverD3D12::get().getCopyCommandQueue()->WaitForFenceValue(uploadFenceValue);

	safe_release(resource);
	safe_release(uploadBuffer);

	DriverD3D12::get().unregisterBuffer(id);
}

void Buffer::updateData(const void* src_data)
{
	// TODO: optimize for dynamic/frequently updated buffers

	const uint64 bufferSize = (uint64)desc.numElements * (uint64)desc.elementByteSize;

	if (uploadBuffer == nullptr)
	{
		const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		verify(DriverD3D12::get().getDevice().CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)));
	}

	D3D12_SUBRESOURCE_DATA subresourceData;
	subresourceData.pData = src_data;
	subresourceData.RowPitch = bufferSize;
	subresourceData.SlicePitch = subresourceData.RowPitch;

	CommandQueue* copyQueue = DriverD3D12::get().getCopyCommandQueue();
	ComPtr<ID3D12GraphicsCommandList2> cmdList = copyQueue->GetCommandList();

	transition(D3D12_RESOURCE_STATE_COPY_DEST, cmdList.Get());

	UpdateSubresources(cmdList.Get(), resource, uploadBuffer, 0, 0, 1, &subresourceData);

	uploadFenceValue = copyQueue->ExecuteCommandList(cmdList.Get());
}

void Buffer::transition(D3D12_RESOURCE_STATES dest_state, ID3D12GraphicsCommandList2* cmd_list)
{
	if (dest_state == resourceState)
		return;

	DriverD3D12::get().transitionResource(resource, resourceState, dest_state, cmd_list);
	resourceState = dest_state;
}

}