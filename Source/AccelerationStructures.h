#pragma once
#include "PCH.h"
#include "MeshData.h"

struct MeshData;
class HeapManager;

D3D12_RAYTRACING_INSTANCE_DESC CreateInstance(ID3D12Resource2* blas, DirectX::XMMATRIX* transform, UINT id, UINT hitGroupIndex);

enum BLASIdentifier
{
	Cube_ID = 0
};

class SceneAccelerationStructure
{
	std::map<BLASIdentifier, Microsoft::WRL::ComPtr<ID3D12Resource2>> BLAS;
	Microsoft::WRL::ComPtr<ID3D12Resource2> TLAS;
};

class TLAS_Generator
{
public:
	TLAS_Generator(ID3D12Device11* device, HeapManager* heap);
	// TODO id and hitGroupIndex need to be ENUMS
	void AddInstance(const D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc);
	void Generate(ID3D12GraphicsCommandList6* commandList, Microsoft::WRL::ComPtr<ID3D12Resource2>& resultTlas);
private:
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_Scratch;
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_DescriptorsBuffer;
	ID3D12Device11* m_Device;
	HeapManager* m_Heap;
	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_InstanceDesc;
};

class BLAS_Generator
{
public:
	BLAS_Generator(ID3D12Device11* device, HeapManager* heap, MeshData* meshdata);
	void Generate(ID3D12GraphicsCommandList6* commandList, Microsoft::WRL::ComPtr<ID3D12Resource2>& resultBlas);
private:
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_VertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_IndexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_Scratch;
	ID3D12Device11* m_Device;
	HeapManager* m_Heap;
	UINT m_IndexCount;
	UINT m_VertexCount;
	UINT m_Stride;
};