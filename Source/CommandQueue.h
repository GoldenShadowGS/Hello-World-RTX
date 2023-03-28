#pragma once
#include "PCH.h"

class CommandQueue
{
public:
	CommandQueue();
	~CommandQueue();
	void Create(ID3D12Device11* device, D3D12_COMMAND_LIST_TYPE type);
	void ExecuteCommandList(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6>& commandList);
	inline ID3D12CommandQueue* GetPtr() { return m_CommandQueue.Get(); }
	UINT64 Signal();
	void WaitForFenceValue(UINT64 fenceValue);
	void Flush();
private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
	Microsoft::WRL::ComPtr<ID3D12Fence1> m_Fence;
	UINT64 m_FenceValue;
	HANDLE m_EventHandle;
};