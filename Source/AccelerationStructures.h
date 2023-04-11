#pragma once
#include "PCH.h"
#include "MeshData.h"

// TODO id and hitGroupIndex need to be ENUMS

struct MeshData;
class HeapManager;

enum BLASIdentifier
{
	MeshCube = 0
};

class SceneAccelerationStructure
{
public:
	void Init(ID3D12Device11* device, HeapManager* heap);
	void Reset();
	void AddMesh(ID3D12GraphicsCommandList6* commandList, BLASIdentifier id, MeshData* mesh);
	void AddInstance(BLASIdentifier id, DirectX::XMMATRIX* transform, UINT instanceID, UINT hitGroupIndex);
	void Build(ID3D12GraphicsCommandList6* commandList);
	inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() { return TLAS->GetGPUVirtualAddress(); }
private:
	ID3D12Device11* m_Device = nullptr;
	HeapManager* m_Heap = nullptr;
	std::map<BLASIdentifier, Microsoft::WRL::ComPtr<ID3D12Resource2>> BLAS;
	Microsoft::WRL::ComPtr<ID3D12Resource2> TLAS;
	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_InstanceDescs;
};

class TLAS_Generator
{
public:
	TLAS_Generator(ID3D12Device11* device, HeapManager* heap);
	void Generate(ID3D12GraphicsCommandList6* commandList, Microsoft::WRL::ComPtr<ID3D12Resource2>& resultTlas, std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& instanceDescs);
private:
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_Scratch;
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_DescriptorsBuffer;
	ID3D12Device11* m_Device;
	HeapManager* m_Heap;
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