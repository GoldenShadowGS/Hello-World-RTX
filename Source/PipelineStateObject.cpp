#include "PCH.h"
#include "PipelineStateObject.h"
#include "DX12Utility.h"

using Microsoft::WRL::ComPtr;

StateObjectGenerator::StateObjectGenerator()
{}

int StateObjectGenerator::AddDXIL_Library(void* bytecode, SIZE_T bytecodeSize, D3D12_EXPORT_DESC* exportdesc, UINT numExports)
{
	D3D12_DXIL_LIBRARY_DESC libDesc = {};
	libDesc.DXILLibrary.pShaderBytecode = bytecode;
	libDesc.DXILLibrary.BytecodeLength = bytecodeSize;
	libDesc.NumExports = numExports;
	libDesc.pExports = exportdesc;
	m_LibDescs.push_back(libDesc);

	D3D12_STATE_SUBOBJECT subobject = {};
	subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobject.pDesc = &m_LibDescs.back();

	PushSubObject(subobject);
	return GetBackIndex();
}

int StateObjectGenerator::AddHitGroup(LPCWSTR HitGroup, D3D12_HIT_GROUP_TYPE type, LPCWSTR anyhit, LPCWSTR closesthit, LPCWSTR intersection)
{
	D3D12_HIT_GROUP_DESC hitgroup = {};
	hitgroup.HitGroupExport = HitGroup;
	hitgroup.Type = type;
	hitgroup.AnyHitShaderImport = anyhit;
	hitgroup.ClosestHitShaderImport = closesthit;
	hitgroup.IntersectionShaderImport = intersection;
	m_HitGroupDescs.push_back(hitgroup);

	D3D12_STATE_SUBOBJECT subobject = {};
	subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	subobject.pDesc = &m_HitGroupDescs.back();

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

int StateObjectGenerator::AddExportsAssociation(LPCWSTR* exports, UINT exportCount, UINT subObjectIndex)
{
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderPayloadAssociation = {};
	shaderPayloadAssociation.pSubobjectToAssociate = nullptr;
	shaderPayloadAssociation.NumExports = exportCount;
	shaderPayloadAssociation.pExports = exports;
	m_ShaderPayloadAssociation.push_back(shaderPayloadAssociation);

	D3D12_STATE_SUBOBJECT subobject = {};
	subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	subobject.pDesc = &m_ShaderPayloadAssociation.back();

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
