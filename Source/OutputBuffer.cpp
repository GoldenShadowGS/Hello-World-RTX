#include "PCH.h"
#include "OutputBuffer.h"

void OutputBuffer::Create(ID3D12Device11* device, UINT width, UINT height, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle)
{
	D3D12_RESOURCE_DESC ResourceDesc = {};
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Alignment = 0;
	ResourceDesc.Width = (UINT64)width;
	ResourceDesc.Height = height;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_RESOURCE_ALLOCATION_INFO alloc_info = device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

	if (m_HeapSize < alloc_info.SizeInBytes)
	{
		CreateHeap(device, alloc_info.SizeInBytes);
	}
	device->CreatePlacedResource(m_Heap.Get(), 0, &ResourceDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&m_Resource));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	//D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_Renderer.m_DescriptorHeap.GetCPUHandle(0);
	device->CreateUnorderedAccessView(m_Resource.Get(), nullptr, &uavDesc, srvHandle);
}

void OutputBuffer::CreateHeap(ID3D12Device11* device, UINT64 heapsize)
{
	m_HeapSize = Align64(heapsize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	D3D12_HEAP_DESC heapProperties = {};
	heapProperties.SizeInBytes = m_HeapSize;
	heapProperties.Properties = { D3D12_HEAP_TYPE_DEFAULT };
	heapProperties.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	heapProperties.Flags = D3D12_HEAP_FLAG_NONE;

	ThrowIfFailed(device->CreateHeap(&heapProperties, IID_PPV_ARGS(&m_Heap)));
}
