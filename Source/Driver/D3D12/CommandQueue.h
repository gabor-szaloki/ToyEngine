#pragma once

#include "DriverCommonD3D12.h"

#include <string>
#include <queue>

namespace drv_d3d12
{
	class CommandQueue
	{
	public:
		CommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
		~CommandQueue();

		// Get an available command list from the command queue.
		ComPtr<ID3D12GraphicsCommandList2> GetCommandList();

		// Execute a command list.
		// Returns the fence value to wait for for this command list.
		uint64 ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList2> commandList);

		uint64 Signal();
		bool IsFenceComplete(uint64 fenceValue);
		void WaitForFenceValue(uint64 fenceValue);
		void Flush();

		ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;

	private:
		ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
		ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator);

		// Keep track of command allocators that are "in-flight"
		struct CommandAllocatorEntry
		{
			uint64 fenceValue;
			ComPtr<ID3D12CommandAllocator> commandAllocator;
		};

		using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
		using CommandListQueue = std::queue<ComPtr<ID3D12GraphicsCommandList2>>;

		D3D12_COMMAND_LIST_TYPE m_CommandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
		ComPtr<ID3D12Device2> m_d3d12Device;
		ComPtr<ID3D12CommandQueue> m_d3d12CommandQueue;
		ComPtr<ID3D12Fence> m_d3d12Fence;
		HANDLE m_FenceEvent = nullptr;
		uint64 m_FenceValue = 0;

		CommandAllocatorQueue m_CommandAllocatorQueue;
		CommandListQueue m_CommandListQueue;
	};
}