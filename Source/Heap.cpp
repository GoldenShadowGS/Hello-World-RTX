#include "PCH.h"
#include "Heap.h"
#include "DX12Utility.h"

using namespace Microsoft::WRL;

HeapManager::SingleHeap::SingleHeap() :
	m_CurrentOffset(0)
{}

void HeapManager::SingleHeap::Create(ID3D12Device11* device, D3D12_HEAP_TYPE type, UINT64 heapsize)
{
	D3D12_HEAP_DESC heapProperties = {};
	heapProperties.SizeInBytes = Align64(heapsize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	heapProperties.Properties = { type };
	heapProperties.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	heapProperties.Flags = D3D12_HEAP_FLAG_NONE;

	ThrowIfFailed(device->CreateHeap(&heapProperties, IID_PPV_ARGS(&m_Heap)));
}

ComPtr<ID3D12Resource2> HeapManager::SingleHeap::CreateResource(ID3D12Device11* device, D3D12_RESOURCE_DESC* desc, D3D12_RESOURCE_STATES state, D3D12_CLEAR_VALUE* clearvalue)
{
	ComPtr<ID3D12Resource2> Resource;
	UINT64 ResourceSize = 0;
	device->GetCopyableFootprints(desc, 0, desc->MipLevels, 0, nullptr, nullptr, nullptr, &ResourceSize);
	ResourceSize = Align64(ResourceSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	ThrowIfFailed(device->CreatePlacedResource(m_Heap.Get(), m_CurrentOffset, desc, state, clearvalue, IID_PPV_ARGS(&Resource)));
	m_CurrentOffset += ResourceSize;
	return Resource;
}

void HeapManager::SingleHeap::Reset()
{
	m_Heap.Reset();
}

void HeapManager::Create(ID3D12Device11* device, UINT64 uploadheapsize, UINT64 defaultheapsize, UINT64 constantbufferheapsize)
{

	m_Heaps[0].Create(device, D3D12_HEAP_TYPE_UPLOAD, uploadheapsize);
	m_Heaps[1].Create(device, D3D12_HEAP_TYPE_DEFAULT, defaultheapsize);
	m_Heaps[2].Create(device, D3D12_HEAP_TYPE_UPLOAD, constantbufferheapsize);

}

ComPtr<ID3D12Resource2> HeapManager::CreateResource(ID3D12Device11* device, HeapType type, D3D12_RESOURCE_DESC* desc, D3D12_RESOURCE_STATES state, D3D12_CLEAR_VALUE* clearvalue)
{
	return m_Heaps[type].CreateResource(device, desc, state, clearvalue);
}

void HeapManager::ResetUploadHeap()
{
	m_Heaps[UploadHeap].Reset();
}