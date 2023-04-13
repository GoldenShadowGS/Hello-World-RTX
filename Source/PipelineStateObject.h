#pragma once
#include "PCH.h"

class StateObjectGenerator
{
public:
	StateObjectGenerator();
	Microsoft::WRL::ComPtr<ID3D12StateObject> Build(ID3D12Device11* device);
	int AddDXIL_Library(void* bytecode, SIZE_T bytecodeSize, const std::vector<std::wstring>& exportNames);
	int AddHitGroup(const std::wstring& hitgroup, D3D12_HIT_GROUP_TYPE type, const std::wstring& anyhit, const std::wstring& closesthit, const std::wstring& intersection);
	int AddShaderConfig(UINT MaxPayloadSizeInBytes, UINT MaxAttributeSizeInBytes);
	int AddLocalRootSignature(ID3D12RootSignature* rootsig);
	int AddGlobalRootSignature(ID3D12RootSignature* rootsig);
	int AddExportsAssociation(const std::vector<std::wstring>& exportNames, UINT subObjectIndex);
	int AddPipelineConfig(UINT MaxTraceRecursionDepth);
private:
	inline void PushSubObject(D3D12_STATE_SUBOBJECT subobject, int associationIndex = -1)
	{
		m_AssociationIndex.push_back(associationIndex);
		m_Subobjects.push_back(subobject);
	}
	inline int GetBackIndex() { return (int)m_Subobjects.size() - 1; }
	std::vector<D3D12_STATE_SUBOBJECT> m_Subobjects;
	std::vector<int> m_AssociationIndex;
	struct DXILLibContainer
	{
		std::vector<std::wstring> m_ExportNames;
		std::vector<D3D12_EXPORT_DESC> m_ExportDescs;
		D3D12_DXIL_LIBRARY_DESC m_LibDesc = {};
	};
	std::list<DXILLibContainer> m_DXILLib;
	struct HitGroupContainer
	{
		std::wstring HitGroupName;
		std::wstring AnyHitName;
		std::wstring ClosestHitName;
		std::wstring IntersectionName;
		D3D12_HIT_GROUP_DESC m_HitGroupDesc = {};
	};
	std::list<HitGroupContainer> m_HitGroupContainers;
	std::list<D3D12_LOCAL_ROOT_SIGNATURE> m_LocalRootSignatures;
	std::list<D3D12_GLOBAL_ROOT_SIGNATURE> m_GlobalRootSignatures;
	struct SubObjectAssociationContainer
	{
		std::vector<std::wstring> m_ExportNames;
		std::vector<LPCWSTR> m_NamePointers;
		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION Association = {};
	};
	std::list<SubObjectAssociationContainer> m_SubobjectAssociations;
	D3D12_RAYTRACING_SHADER_CONFIG m_Shaderconfig = {};
	D3D12_RAYTRACING_PIPELINE_CONFIG m_PipelineConfig = {};
};
