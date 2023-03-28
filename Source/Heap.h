#pragma once
#include "PCH.h"

enum HeapType
{
	UploadHeap = 0,
	DefaultHeap = 1,
	ConstantBufferHeap = 2
};

class HeapManager
{
	class SingleHeap
	{
	public:
		SingleHeap();
		void Create(ID3D12Device11* device, D3D12_HEAP_TYPE type, UINT64 heapsize);
		Microsoft::WRL::ComPtr<ID3D12Resource2> CreateResource(ID3D12Device11* device, D3D12_RESOURCE_DESC* desc, D3D12_RESOURCE_STATES state, D3D12_CLEAR_VALUE* clearvalue);
		void Reset();
	private:
		UINT64 m_CurrentOffset;
		Microsoft::WRL::ComPtr<ID3D12Heap> m_Heap;
	};
public:
	void Create(ID3D12Device11* device, UINT64 uploadheapsize, UINT64 defaultheapsize, UINT64 constantbufferheapsize);
	Microsoft::WRL::ComPtr<ID3D12Resource2> CreateResource(ID3D12Device11* device, HeapType type, D3D12_RESOURCE_DESC* desc, D3D12_RESOURCE_STATES state, D3D12_CLEAR_VALUE* clearvalue = nullptr);
	void ResetUploadHeap();
private:
	SingleHeap m_Heaps[3];
};