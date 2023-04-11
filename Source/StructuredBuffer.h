#pragma once
#include "PCH.h"

class DescriptorHeap;
class HeapManager;

class StructuredBuffer
{
public:
	void CreateResource(ID3D12Device11* device, HeapManager* heap, DescriptorHeap* descriptorHeap, UINT64 size, UINT handle);
	void Upload(void* source, UINT64 size);
private:
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_Resource;
	void* m_Destination = nullptr;
	UINT64 m_Size = 0;
};