#pragma once
#include "PCH.h"

class DescriptorHeap
{
public:
	DescriptorHeap();
	void Create(ID3D12Device11* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescs, D3D12_DESCRIPTOR_HEAP_FLAGS flags);
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT index);
	inline ID3D12DescriptorHeap* GetHeap() { return m_DescriptorHeap.Get(); }
	inline ID3D12DescriptorHeap** GetAddressOfHeap() { return m_DescriptorHeap.GetAddressOf(); }
	inline UINT GetUniqueHeapIndex() { return HEAPINDEX++; }
private:
	static UINT HEAPINDEX;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHeapStart;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHeapStart;
	UINT HandleIncrementSize;
};