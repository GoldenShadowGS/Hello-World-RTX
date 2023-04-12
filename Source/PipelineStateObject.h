#pragma once
#include "PCH.h"



class StateObjectGenerator
{
public:
	StateObjectGenerator();
	int AddDXIL_Library(void* bytecode, SIZE_T bytecodeSize, D3D12_EXPORT_DESC* exportdesc, UINT numExports);
	int AddHitGroup(LPCWSTR HitGroup, D3D12_HIT_GROUP_TYPE type, LPCWSTR anyhit, LPCWSTR closesthit, LPCWSTR intersection);
	int AddShaderConfig(UINT MaxPayloadSizeInBytes, UINT MaxAttributeSizeInBytes);
	int AddLocalRootSignature(ID3D12RootSignature* rootsig);
	int AddGlobalRootSignature(ID3D12RootSignature* rootsig);
	int AddExportsAssociation(LPCWSTR* exports, UINT exportCount, UINT subObjectIndex);
	int AddPipelineConfig(UINT MaxTraceRecursionDepth);
	Microsoft::WRL::ComPtr<ID3D12StateObject> Build(ID3D12Device11* device);
private:
	inline void PushSubObject(D3D12_STATE_SUBOBJECT subobject, int associationIndex = -1)
	{
		m_AssociationIndex.push_back(associationIndex);
		m_Subobjects.push_back(subobject);
	}
	inline int GetBackIndex() { return (int)m_Subobjects.size() - 1; }
	std::vector<D3D12_STATE_SUBOBJECT> m_Subobjects;
	std::vector<int> m_AssociationIndex;
	std::list<D3D12_DXIL_LIBRARY_DESC> m_LibDescs;
	std::list<D3D12_HIT_GROUP_DESC> m_HitGroupDescs;
	std::list<D3D12_LOCAL_ROOT_SIGNATURE> m_LocalRootSignatures;
	std::list<D3D12_GLOBAL_ROOT_SIGNATURE> m_GlobalRootSignatures;
	std::list<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> m_ShaderPayloadAssociation;
	D3D12_RAYTRACING_SHADER_CONFIG m_Shaderconfig = {};
	D3D12_RAYTRACING_PIPELINE_CONFIG m_PipelineConfig = {};
};
