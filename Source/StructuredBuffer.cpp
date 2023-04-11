#pragma once
#include "PCH.h"
#include "StructuredBuffer.h"
#include "DescriptorHeap.h"
#include "Heap.h"
#include "DX12Utility.h"


void StructuredBuffer::CreateResource(ID3D12Device11* device, HeapManager* heap, DescriptorHeap* descriptorHeap, UINT64 size, UINT handle)
{
	m_Size = Align64(size, 256);

	m_Resource = heap->CreateBufferResource(device, UploadHeap, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, m_Size);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc = {};
	cbDesc.BufferLocation = m_Resource->GetGPUVirtualAddress();
	cbDesc.SizeInBytes = (UINT)m_Size;
	D3D12_CPU_DESCRIPTOR_HANDLE cbHandle = descriptorHeap->GetCPUHandle(handle);
	device->CreateConstantBufferView(&cbDesc, cbHandle);

	const D3D12_RANGE readRange = { 0, 0 };
	ThrowIfFailed(m_Resource->Map(0, &readRange, &m_Destination));
}

void StructuredBuffer::Upload(void* source, UINT64 size)
{
	assert(size <= m_Size);
	memcpy(m_Destination, source, size);
}