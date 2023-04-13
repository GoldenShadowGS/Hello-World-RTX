#include "PCH.h"
#include "PipelineStateObject.h"
#include "DX12Utility.h"

using Microsoft::WRL::ComPtr;

StateObjectGenerator::StateObjectGenerator()
{}

int StateObjectGenerator::AddDXIL_Library(void* bytecode, SIZE_T bytecodeSize, const std::vector<std::wstring>& exportNames)
{
	DXILLibContainer l;
	m_DXILLib.push_back(l);
	DXILLibContainer& lib = m_DXILLib.back();
	lib.m_ExportNames = exportNames;
	lib.m_ExportDescs.reserve(lib.m_ExportNames.size());
	const int size = (int)lib.m_ExportNames.size();
	for (int i = 0; i < size; i++)
	{
		lib.m_ExportDescs.emplace_back(lib.m_ExportNames[i].c_str());
	}

	lib.m_LibDesc.DXILLibrary.pShaderBytecode = bytecode;
	lib.m_LibDesc.DXILLibrary.BytecodeLength = bytecodeSize;
	lib.m_LibDesc.NumExports = (UINT)exportNames.size();
	lib.m_LibDesc.pExports = lib.m_ExportDescs.data();

	D3D12_STATE_SUBOBJECT subobject = {};
	subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobject.pDesc = &lib.m_LibDesc;

	PushSubObject(subobject);
	return GetBackIndex();
}

int StateObjectGenerator::AddHitGroup(const std::wstring& hitgroup, D3D12_HIT_GROUP_TYPE type, const std::wstring& anyhit, const std::wstring& closesthit, const std::wstring& intersection)
{
	HitGroupContainer hc;
	m_HitGroupContainers.push_back(hc);
	HitGroupContainer& HGC = m_HitGroupContainers.back();

	HGC.HitGroupName = hitgroup;
	HGC.AnyHitName = anyhit;
	HGC.ClosestHitName = closesthit;
	HGC.IntersectionName = intersection;

	HGC.m_HitGroupDesc.HitGroupExport = HGC.HitGroupName.empty() ? nullptr : HGC.HitGroupName.c_str();
	HGC.m_HitGroupDesc.Type = type;
	HGC.m_HitGroupDesc.AnyHitShaderImport = HGC.AnyHitName.empty() ? nullptr : HGC.AnyHitName.c_str();
	HGC.m_HitGroupDesc.ClosestHitShaderImport = HGC.ClosestHitName.empty() ? nullptr : HGC.ClosestHitName.c_str();
	HGC.m_HitGroupDesc.IntersectionShaderImport = HGC.IntersectionName.empty() ? nullptr : HGC.IntersectionName.c_str();

	D3D12_STATE_SUBOBJECT subobject = {};
	subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	subobject.pDesc = &m_HitGroupContainers.back().m_HitGroupDesc;

	PushSubObject(subobject);
	return GetBackIndex();
}

int StateObjectGenerator::AddShaderConfig(UINT maxPayloadSizeInBytes, UINT maxAttributeSizeInBytes)
{
	m_Shaderconfig.MaxPayloadSizeInBytes = maxPayloadSizeInBytes;
	m_Shaderconfig.MaxAttributeSizeInBytes = maxAttributeSizeInBytes;

	D3D12_STATE_SUBOBJECT subobject = {};
	subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	subobject.pDesc = &m_Shaderconfig;

	PushSubObject(subobject);
	return GetBackIndex();
}

int StateObjectGenerator::AddLocalRootSignature(ID3D12RootSignature* rootsig)
{
	D3D12_LOCAL_ROOT_SIGNATURE rootSignature = {};
	rootSignature.pLocalRootSignature = rootsig;
	m_LocalRootSignatures.push_back(rootSignature);

	D3D12_STATE_SUBOBJECT subobject = {};
	subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	subobject.pDesc = &m_LocalRootSignatures.back();

	PushSubObject(subobject);
	return GetBackIndex();
}

int StateObjectGenerator::AddGlobalRootSignature(ID3D12RootSignature* rootsig)
{
	D3D12_GLOBAL_ROOT_SIGNATURE rootSignature = {};
	rootSignature.pGlobalRootSignature = rootsig;
	m_GlobalRootSignatures.push_back(rootSignature);

	D3D12_STATE_SUBOBJECT subobject = {};
	subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	subobject.pDesc = &m_GlobalRootSignatures.back();

	PushSubObject(subobject);
	return GetBackIndex();
}

int StateObjectGenerator::AddExportsAssociation(const std::vector<std::wstring>& exportNames, UINT subObjectIndex)
{
	SubObjectAssociationContainer s;
	m_SubobjectAssociations.push_back(s);
	SubObjectAssociationContainer& so = m_SubobjectAssociations.back();

	so.m_ExportNames = exportNames;
	so.m_NamePointers.reserve(so.m_ExportNames.size());

	const int size = (int)so.m_ExportNames.size();
	for (int i = 0; i < size; i++)
	{
		so.m_NamePointers.emplace_back(so.m_ExportNames[i].c_str());
	}

	so.Association.pSubobjectToAssociate = nullptr;
	so.Association.NumExports = (UINT)so.m_ExportNames.size();
	so.Association.pExports = so.m_NamePointers.data();

	D3D12_STATE_SUBOBJECT subobject = {};
	subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	subobject.pDesc = &m_SubobjectAssociations.back().Association;

	PushSubObject(subobject, subObjectIndex);
	return GetBackIndex();
}

int StateObjectGenerator::AddPipelineConfig(UINT MaxTraceRecursionDepth)
{
	m_PipelineConfig.MaxTraceRecursionDepth = MaxTraceRecursionDepth;

	D3D12_STATE_SUBOBJECT subobject = {};
	subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	subobject.pDesc = &m_PipelineConfig;

	PushSubObject(subobject);
	return GetBackIndex();
}

ComPtr<ID3D12StateObject> StateObjectGenerator::Build(ID3D12Device11* device)
{
	assert(m_Subobjects.size() == m_AssociationIndex.size());
	const int size = (int)m_Subobjects.size();

	// Restore association pointers
	for (int i = 0; i < size; i++)
	{
		if (m_Subobjects[i].Type == D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION)
		{
			int index = m_AssociationIndex[i];
			assert(index != -1);
			D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* associationPtr = (D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*)m_Subobjects[i].pDesc;
			associationPtr->pSubobjectToAssociate = &m_Subobjects[index];
		}
	}

	D3D12_STATE_OBJECT_DESC pipelineDesc = {};
	pipelineDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	pipelineDesc.NumSubobjects = (UINT)m_Subobjects.size();
	pipelineDesc.pSubobjects = m_Subobjects.data();

	ComPtr<ID3D12StateObject> StateObject;
	ThrowIfFailed(device->CreateStateObject(&pipelineDesc, IID_PPV_ARGS(&StateObject)));

	return StateObject;
}
