#include "CommandQueue.h"

namespace drv_d3d12
{

CommandQueue::CommandQueue(ID3D12Device2* device, D3D12_COMMAND_LIST_TYPE type)
	: commandListType(type)
	, d3d12Device(device)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	verify(d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));
	verify(d3d12Device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent && "Failed to create fence event handle.");
}

CommandQueue::~CommandQueue()
{
	while (!commandAllocatorQueue.empty())
	{
		commandAllocatorQueue.front().commandAllocator->Release();
		commandAllocatorQueue.pop();
	}
	while (!commandListQueue.empty())
	{
		commandListQueue.front()->Release();
		commandListQueue.pop();
	}

	::CloseHandle(fenceEvent);
	fence->Release();
	d3d12CommandQueue->Release();
}

ID3D12GraphicsCommandList2* CommandQueue::GetCommandList()
{
	ID3D12CommandAllocator* commandAllocator;
	ID3D12GraphicsCommandList2* commandList;

	{
		std::lock_guard<std::mutex> lock(mutex);

		// Fetch and reset an available CommandAllocator, or create new one if there is none available
		if (!commandAllocatorQueue.empty() && IsFenceComplete(commandAllocatorQueue.front().fenceValue))
		{
			commandAllocator = commandAllocatorQueue.front().commandAllocator;
			commandAllocatorQueue.pop();

			verify(commandAllocator->Reset());
		}
		else
		{
			commandAllocator = CreateCommandAllocator();
		}

		// Fetch and reset an available CommandList, or create a new one if there is none available
		if (!commandListQueue.empty())
		{
			commandList = commandListQueue.front();
			commandListQueue.pop();

			verify(commandList->Reset(commandAllocator, nullptr));
		}
		else
		{
			commandList = CreateCommandList(commandAllocator);
		}
	}

	// Associate the command allocator with the command list so that it can be retrieved when the command list is executed.
	verify(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator));

	return commandList;
}

uint64 CommandQueue::ExecuteCommandList(ID3D12GraphicsCommandList2* command_list)
{
	command_list->Close();

	ID3D12CommandAllocator* commandAllocator;
	UINT dataSize = sizeof(commandAllocator);
	verify(command_list->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

	ID3D12CommandList* const ppCommandLists[] = { command_list };

	d3d12CommandQueue->ExecuteCommandLists(1, ppCommandLists);
	uint64_t fenceValue = Signal();

	{
		std::lock_guard<std::mutex> lock(mutex);
	
		commandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue, commandAllocator });
		commandListQueue.push(command_list);
	}

	// The ownership of the command allocator has been transferred to the ComPtr
	// in the command allocator queue. It is safe to release the reference 
	// in this temporary COM pointer here.
	commandAllocator->Release();

	return fenceValue;
}

uint64 CommandQueue::Signal()
{
	verify(d3d12CommandQueue->Signal(fence, ++fenceValue));
	return fenceValue;
}

bool CommandQueue::IsFenceComplete(uint64 fence_value)
{
	return fence->GetCompletedValue() >= fence_value;
}

void CommandQueue::WaitForFenceValue(uint64 fence_value)
{
	if (!IsFenceComplete(fence_value))
	{
		verify(fence->SetEventOnCompletion(fence_value, fenceEvent));
		::WaitForSingleObject(fenceEvent, DWORD_MAX);
	}
}

void CommandQueue::Flush()
{
	WaitForFenceValue(Signal());
}

ID3D12CommandAllocator* CommandQueue::CreateCommandAllocator()
{
	ID3D12CommandAllocator* commandAllocator;
	verify(d3d12Device->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

ID3D12GraphicsCommandList2* CommandQueue::CreateCommandList(ID3D12CommandAllocator* allocator)
{
	ID3D12GraphicsCommandList2* commandList;
	verify(d3d12Device->CreateCommandList(0, commandListType, allocator, nullptr, IID_PPV_ARGS(&commandList)));

	return commandList;
}

} // namespace drv_d3d12