#include "PCH.h"
#include "AccelerationStructures.h"
#include "Heap.h"
#include "MeshData.h"
#include "DX12Utility.h"

using Microsoft::WRL::ComPtr;

// TLAS_Generator

TLAS_Generator::TLAS_Generator(ID3D12Device11* device, HeapManager* heap) :
	m_Device(device),
	m_Heap(heap)
{}

D3D12_RAYTRACING_INSTANCE_DESC CreateInstance(ID3D12Resource2* blas, DirectX::XMMATRIX* transform, UINT id, UINT hitGroupIndex)
{
	DirectX::XMMATRIX matrix = XMMatrixTranspose(*transform);
	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	memcpy(&instanceDesc.Transform, &matrix, sizeof(instanceDesc.Transform));
	instanceDesc.InstanceID = id; // Instance ID visible in the shader in InstanceID()
	instanceDesc.InstanceMask = 0xFF; // Visibility mask, always visible here
	instanceDesc.InstanceContributionToHitGroupIndex = hitGroupIndex; // Index of the hit group invoked upon intersection
	instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	instanceDesc.AccelerationStructure = blas->GetGPUVirtualAddress();
	return instanceDesc;
}

void TLAS_Generator::AddInstance(const D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc)
{
	m_InstanceDesc.push_back(instanceDesc);
}

void TLAS_Generator::Generate(ID3D12GraphicsCommandList6* commandList, ComPtr<ID3D12Resource2>& resultTlas)
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuildDesc = {};
	prebuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	prebuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	prebuildDesc.NumDescs = (UINT)m_InstanceDesc.size();
	prebuildDesc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info = {};

	m_Device->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildDesc, &prebuild_info);

	const UINT64 instanceDescsSize = Align64(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * m_InstanceDesc.size(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	const UINT64 scratchSize = Align64(prebuild_info.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	const UINT64 resultSize = Align64(prebuild_info.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	m_Scratch = m_Heap->CreateBufferResource(m_Device, ScratchDefaultHeap, D3D12_RESOURCE_STATE_COMMON, scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	resultTlas = m_Heap->CreateBufferResource(m_Device, TLASHeap, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	m_DescriptorsBuffer = m_Heap->CreateBufferResource(m_Device, ScratchUploadHeap, D3D12_RESOURCE_STATE_GENERIC_READ, instanceDescsSize);

	void* Destination = nullptr;
	const UINT Size = (UINT)sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * (UINT)m_InstanceDesc.size();
	const D3D12_RANGE readRange = { 0, 0 };
	ThrowIfFailed(m_DescriptorsBuffer->Map(0, &readRange, &Destination));
	memcpy(Destination, m_InstanceDesc.data(), Size);
	m_DescriptorsBuffer->Unmap(0, nullptr);

	// Create a descriptor of the requested builder work, to generate a top-level AS from the input parameters
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.DestAccelerationStructureData = { resultTlas->GetGPUVirtualAddress() };
	buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	buildDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	buildDesc.Inputs.NumDescs = prebuildDesc.NumDescs;
	buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	buildDesc.Inputs.InstanceDescs = m_DescriptorsBuffer->GetGPUVirtualAddress();
	buildDesc.ScratchAccelerationStructureData = m_Scratch->GetGPUVirtualAddress();
	buildDesc.SourceAccelerationStructureData = 0;

	// Build the top-level AS
	commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	// Wait for the builder to complete by setting a barrier on the resulting
	// buffer. This can be important in case the rendering is triggered
	// immediately afterwards, without executing the command list
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = resultTlas.Get();
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	commandList->ResourceBarrier(1, &uavBarrier);
}



// BLAS_Generator

BLAS_Generator::BLAS_Generator(ID3D12Device11* device, HeapManager* heap, MeshData* meshdata) :
	m_Device(device),
	m_Heap(heap),
	m_IndexCount((UINT)meshdata->m_Indices.size()),
	m_VertexCount((UINT)meshdata->m_Vertices.size()),
	m_Stride(meshdata->stride)
{
	UINT64 sizeinBytes = (UINT64)m_VertexCount * m_Stride;
	m_VertexBuffer = m_Heap->CreateBufferResource(m_Device, ScratchUploadHeap, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, sizeinBytes);
	CopyDataToUploadResource(meshdata->m_Vertices.data(), m_VertexBuffer.Get(), sizeinBytes);

	sizeinBytes = m_IndexCount * sizeof(UINT);
	m_IndexBuffer = m_Heap->CreateBufferResource(m_Device, ScratchUploadHeap, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, sizeinBytes);
	CopyDataToUploadResource(meshdata->m_Indices.data(), m_IndexBuffer.Get(), sizeinBytes);
}

void BLAS_Generator::Generate(ID3D12GraphicsCommandList6* commandList, ComPtr<ID3D12Resource2>& resultBlas)
{
	D3D12_RAYTRACING_GEOMETRY_DESC geometry_Desc = {};
	geometry_Desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometry_Desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	geometry_Desc.Triangles.Transform3x4 = 0;
	geometry_Desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
	geometry_Desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometry_Desc.Triangles.IndexCount = m_IndexCount;
	geometry_Desc.Triangles.VertexCount = m_VertexCount;
	geometry_Desc.Triangles.IndexBuffer = m_IndexBuffer->GetGPUVirtualAddress();
	geometry_Desc.Triangles.VertexBuffer.StartAddress = m_VertexBuffer->GetGPUVirtualAddress();
	geometry_Desc.Triangles.VertexBuffer.StrideInBytes = m_Stride;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS AS_Inputs = {};
	AS_Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	AS_Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	AS_Inputs.NumDescs = 1;
	AS_Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	AS_Inputs.pGeometryDescs = &geometry_Desc;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info = {};

	m_Device->GetRaytracingAccelerationStructurePrebuildInfo(&AS_Inputs, &prebuild_info);

	UINT64 scratchSize = Align64(prebuild_info.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	UINT64 resultSize = Align64(prebuild_info.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	m_Scratch = m_Heap->CreateBufferResource(m_Device, ScratchDefaultHeap, D3D12_RESOURCE_STATE_COMMON, scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	resultBlas = m_Heap->CreateBufferResource(m_Device, BLASHeap, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AS_BuildDesc = {};
	AS_BuildDesc.DestAccelerationStructureData = resultBlas->GetGPUVirtualAddress();
	AS_BuildDesc.Inputs = AS_Inputs;
	AS_BuildDesc.SourceAccelerationStructureData = 0;
	AS_BuildDesc.ScratchAccelerationStructureData = m_Scratch->GetGPUVirtualAddress();

	commandList->BuildRaytracingAccelerationStructure(&AS_BuildDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = resultBlas.Get();
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	commandList->ResourceBarrier(1, &uavBarrier);
}
