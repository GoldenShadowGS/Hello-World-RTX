#pragma once
#include "PCH.h"

enum HeapType
{
	UploadHeap = 0,
	ScratchUploadHeap = 1,
	DefaultHeap = 2,
	ScratchDefaultHeap = 3,
	BLASHeap = 4,
	TLASHeap = 5
};

class HeapManager
{
	class SingleHeap
	{
	public:
		SingleHeap();
		void Create(ID3D12Device11* device, D3D12_HEAP_TYPE type, UINT64 heapsize);
		Microsoft::WRL::ComPtr<ID3D12Resource2> CreateResource(ID3D12Device11* device, D3D12_RESOURCE_DESC* desc, D3D12_RESOURCE_STATES state, D3D12_CLEAR_VALUE* clearvalue);
		void ResetOffset();
	private:
		UINT64 m_CurrentOffset;
		Microsoft::WRL::ComPtr<ID3D12Heap> m_Heap;
	};
public:
	void Create(ID3D12Device11* device);
	Microsoft::WRL::ComPtr<ID3D12Resource2> CreateResource(ID3D12Device11* device, HeapType type, D3D12_RESOURCE_DESC* desc, D3D12_RESOURCE_STATES state, D3D12_CLEAR_VALUE* clearvalue = nullptr);
	Microsoft::WRL::ComPtr<ID3D12Resource2> CreateBufferResource(ID3D12Device11* device, HeapType type, D3D12_RESOURCE_STATES state, UINT64 size, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	void ResetHeapOffset(HeapType type);
private:
	SingleHeap m_Heaps[6];
};