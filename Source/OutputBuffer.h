#pragma once
#include "PCH.h"
#include "DX12Utility.h"

class OutputBuffer
{
public:
	void Create(ID3D12Device11* device, UINT width, UINT height, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle);
	inline void Transition(ID3D12GraphicsCommandList6* commandList, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
	{
		TransitionResource(commandList, m_Resource.Get(), beforeState, afterState);
	}
	inline ID3D12Resource2* GetResource() { return m_Resource.Get(); }
private:
	void CreateHeap(ID3D12Device11* device, UINT64 heapsize);
	UINT64 m_HeapSize = 0;
	Microsoft::WRL::ComPtr<ID3D12Heap> m_Heap;
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_Resource;
};
