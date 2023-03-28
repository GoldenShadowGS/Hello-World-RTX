#include "PCH.h"
#include "CommandQueue.h"

using Microsoft::WRL::ComPtr;

CommandQueue::CommandQueue() : m_FenceValue(0), m_EventHandle {}
{}

CommandQueue::~CommandQueue()
{
	if (m_EventHandle)
	{
		CloseHandle(m_EventHandle);
	}
}

void CommandQueue::Create(ID3D12Device11* device, D3D12_COMMAND_LIST_TYPE type)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_CommandQueue)));

	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
	m_EventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(m_EventHandle != INVALID_HANDLE_VALUE);
}

void CommandQueue::ExecuteCommandList(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6>& commandList)
{
	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

UINT64 CommandQueue::Signal()
{
	m_FenceValue++;
	UINT64 fenceSignalValue = m_FenceValue;
	ThrowIfFailed(m_CommandQueue->Signal(m_Fence.Get(), fenceSignalValue));
	return fenceSignalValue;
}

void CommandQueue::WaitForFenceValue(UINT64 fenceValue)
{
	if (m_Fence->GetCompletedValue() < m_FenceValue)
	{
		ThrowIfFailed(m_Fence->SetEventOnCompletion(m_FenceValue, m_EventHandle));
		WaitForSingleObject(m_EventHandle, INFINITE);
	}
}

void CommandQueue::Flush()
{
	UINT64 fenceValueForSignal = Signal();
	WaitForFenceValue(fenceValueForSignal);
}
