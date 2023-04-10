#pragma once
#include "PCH.h"
#include "MeshData.h"

struct MeshData;
class HeapManager;

class TLAS_Generator
{
public:
	TLAS_Generator(ID3D12Device11* device, HeapManager* heap);
	void AddBLAS(ID3D12Resource2* blas, DirectX::XMMATRIX* matrix);
	Microsoft::WRL::ComPtr<ID3D12Resource2> Generate(ID3D12GraphicsCommandList6* commandList);
private:
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_Scratch;		// Scratch memory for AS builder 
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_Result;		// Where the AS is 
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_DescriptorsBuffer;
	ID3D12Device11* m_Device;
	HeapManager* m_Heap;
	struct BLAS_Ptr_and_Transform
	{
		ID3D12Resource2* blas;
		DirectX::XMMATRIX* matrix;
	};
	std::vector<BLAS_Ptr_and_Transform> BLASs;
};

class BLAS_Generator
{
public:
	BLAS_Generator(ID3D12Device11* device, HeapManager* heap, MeshData* meshdata);
	Microsoft::WRL::ComPtr<ID3D12Resource2> Generate(ID3D12GraphicsCommandList6* commandList);
private:
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_VertexBuffer;	// Holds the Vertices
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_IndexBuffer;	// Holds the indices
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_Scratch;		// Scratch memory for AS builder 
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_Result;		// Where the AS is 
	ID3D12Device11* m_Device;
	HeapManager* m_Heap;
	UINT m_IndexCount;
	UINT m_VertexCount;
	UINT m_Stride;
};