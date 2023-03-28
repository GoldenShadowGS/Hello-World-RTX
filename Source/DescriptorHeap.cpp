#include "PCH.h"
#include "DescriptorHeap.h"

UINT DescriptorHeap::HEAPINDEX = 0;

DescriptorHeap::DescriptorHeap() :
	m_CPUHeapStart {},
	m_GPUHeapStart {},
	HandleIncrementSize {}
{}

void DescriptorHeap::Create(ID3D12Device11* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescs, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescs;
	desc.Type = type;
	desc.Flags = flags;
	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_DescriptorHeap)));
	HandleIncrementSize = device->GetDescriptorHandleIncrementSize(desc.Type);
	m_CPUHeapStart = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	if (flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		m_GPUHeapStart = m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUHandle(UINT index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHeapStart = m_CPUHeapStart;
	CPUHeapStart.ptr += (size_t)HandleIncrementSize * (size_t)index;
	return CPUHeapStart;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUHandle(UINT index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE GPUHeapStart = m_GPUHeapStart;
	GPUHeapStart.ptr += (size_t)HandleIncrementSize * (size_t)index;
	return GPUHeapStart;
}

