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

void HeapManager::SingleHeap::ResetOffset()
{
	m_CurrentOffset = 0;
}

void HeapManager::Create(ID3D12Device11* device)
{
	constexpr UINT64 UploadHeapSize = 1024ULL * 1024 * 20;
	constexpr UINT64 ScratchUploadHeapSize = 1024ULL * 1024 * 20;
	constexpr UINT64 DefaultHeapSize = 1024ULL * 1024 * 20;
	constexpr UINT64 ScratchDefaultHeapSize = 1024ULL * 1024 * 20;
	constexpr UINT64 BLASHeapSize = 1024ULL * 1024 * 20;
	constexpr UINT64 TLASHeapSize = 1024ULL * 1024 * 20;
	m_Heaps[UploadHeap].Create(device, D3D12_HEAP_TYPE_UPLOAD, UploadHeapSize);
	m_Heaps[ScratchUploadHeap].Create(device, D3D12_HEAP_TYPE_UPLOAD, ScratchUploadHeapSize);
	m_Heaps[DefaultHeap].Create(device, D3D12_HEAP_TYPE_DEFAULT, DefaultHeapSize);
	m_Heaps[ScratchDefaultHeap].Create(device, D3D12_HEAP_TYPE_DEFAULT, ScratchDefaultHeapSize);
	m_Heaps[BLASHeap].Create(device, D3D12_HEAP_TYPE_DEFAULT, BLASHeapSize);
	m_Heaps[TLASHeap].Create(device, D3D12_HEAP_TYPE_DEFAULT, TLASHeapSize);
}

ComPtr<ID3D12Resource2> HeapManager::CreateResource(ID3D12Device11* device, HeapType type, D3D12_RESOURCE_DESC* desc, D3D12_RESOURCE_STATES state, D3D12_CLEAR_VALUE* clearvalue)
{
	return m_Heaps[type].CreateResource(device, desc, state, clearvalue);
}

ComPtr<ID3D12Resource2> HeapManager::CreateBufferResource(ID3D12Device11* device, HeapType type, D3D12_RESOURCE_STATES state, UINT64 size, D3D12_RESOURCE_FLAGS flags)
{
	D3D12_RESOURCE_DESC ResourceDesc = {};
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Alignment = 0;
	ResourceDesc.Width = size;
	ResourceDesc.Height = 1;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.Flags = flags;
	return CreateResource(device, type, &ResourceDesc, state);
}

// TODO make this generic
void HeapManager::ResetHeapOffset(HeapType type)
{
	m_Heaps[type].ResetOffset();
}