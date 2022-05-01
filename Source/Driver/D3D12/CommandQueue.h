#pragma once

#include "DriverCommonD3D12.h"

#include <queue>
#include <mutex>

namespace drv_d3d12
{
	class CommandQueue
	{
	public:
		CommandQueue(ID3D12Device2* device, D3D12_COMMAND_LIST_TYPE type);
		~CommandQueue();

		// Get an available command list from the command queue.
		ID3D12GraphicsCommandList2* GetCommandList();

		// Execute a command list.
		// Returns the fence value to wait for for this command list.
		uint64 ExecuteCommandList(ID3D12GraphicsCommandList2* command_list);

		uint64 Signal();
		bool IsFenceComplete(uint64 fence_value);
		void WaitForFenceValue(uint64 fence_value);
		void Flush();

		ID3D12CommandQueue* GetD3D12CommandQueue() const { return d3d12CommandQueue; };

	private:
		ID3D12CommandAllocator* CreateCommandAllocator();
		ID3D12GraphicsCommandList2* CreateCommandList(ID3D12CommandAllocator* allocator);

		// Keep track of command allocators that are "in-flight"
		struct CommandAllocatorEntry
		{
			uint64 fenceValue;
			ID3D12CommandAllocator* commandAllocator;
		};

		using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
		using CommandListQueue = std::queue<ID3D12GraphicsCommandList2*>;

		D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
		ID3D12Device2* d3d12Device;
		ID3D12CommandQueue* d3d12CommandQueue;
		ID3D12Fence* fence;
		HANDLE fenceEvent = nullptr;
		uint64 fenceValue = 0;

		CommandAllocatorQueue commandAllocatorQueue;
		CommandListQueue commandListQueue;

		std::mutex mutex;
	};
}