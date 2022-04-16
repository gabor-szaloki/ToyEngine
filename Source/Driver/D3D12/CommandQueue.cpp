#include "CommandQueue.h"

namespace drv_d3d12
{

CommandQueue::CommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
	: m_CommandListType(type)
	, m_d3d12Device(device)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	verify(m_d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_d3d12CommandQueue)));
	verify(m_d3d12Device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_d3d12Fence)));

	m_FenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(m_FenceEvent && "Failed to create fence event handle.");
}

CommandQueue::~CommandQueue()
{
	::CloseHandle(m_FenceEvent);
}

ComPtr<ID3D12GraphicsCommandList2> CommandQueue::GetCommandList()
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList2> commandList;

	// Fetch and reset an available CommandAllocator, or create new one if there is none available
	if (!m_CommandAllocatorQueue.empty() && IsFenceComplete(m_CommandAllocatorQueue.front().fenceValue))
	{
		commandAllocator = m_CommandAllocatorQueue.front().commandAllocator;
		m_CommandAllocatorQueue.pop();

		verify(commandAllocator->Reset());
	}
	else
	{
		commandAllocator = CreateCommandAllocator();
	}

	// Fetch and reset an available CommandList, or create a new one if there is none available
	if (!m_CommandListQueue.empty())
	{
		commandList = m_CommandListQueue.front();
		m_CommandListQueue.pop();

		verify(commandList->Reset(commandAllocator.Get(), nullptr));
	}
	else
	{
		commandList = CreateCommandList(commandAllocator);
	}

	// Associate the command allocator with the command list so that it can be retrieved when the command list is executed.
	verify(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

	return commandList;
}

uint64 CommandQueue::ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	commandList->Close();

	ID3D12CommandAllocator* commandAllocator;
	UINT dataSize = sizeof(commandAllocator);
	verify(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

	ID3D12CommandList* const ppCommandLists[] = { commandList.Get() };

	m_d3d12CommandQueue->ExecuteCommandLists(1, ppCommandLists);
	uint64_t fenceValue = Signal();

	m_CommandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue, commandAllocator });
	m_CommandListQueue.push(commandList);

	// The ownership of the command allocator has been transferred to the ComPtr
	// in the command allocator queue. It is safe to release the reference 
	// in this temporary COM pointer here.
	commandAllocator->Release();

	return fenceValue;
}

uint64 CommandQueue::Signal()
{
	verify(m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), ++m_FenceValue));
	return m_FenceValue;
}

bool CommandQueue::IsFenceComplete(uint64 fenceValue)
{
	return m_d3d12Fence->GetCompletedValue() >= fenceValue;
}

void CommandQueue::WaitForFenceValue(uint64 fenceValue)
{
	if (!IsFenceComplete(fenceValue))
	{
		verify(m_d3d12Fence->SetEventOnCompletion(fenceValue, m_FenceEvent));
		::WaitForSingleObject(m_FenceEvent, DWORD_MAX);
	}
}

void CommandQueue::Flush()
{
	WaitForFenceValue(Signal());
}

ComPtr<ID3D12CommandQueue> CommandQueue::GetD3D12CommandQueue() const
{
	return m_d3d12CommandQueue;
}

ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator()
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	verify(m_d3d12Device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList2> CommandQueue::CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator)
{
	ComPtr<ID3D12GraphicsCommandList2> commandList;
	verify(m_d3d12Device->CreateCommandList(0, m_CommandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

	return commandList;
}

} // namespace drv_d3d12