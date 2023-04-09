#include "PCH.h"
#include "RootSignatureGenerator.h"
#include "DX12Utility.h"

using Microsoft::WRL::ComPtr;

RootSignatureGenerator::RootSignatureGenerator(ID3D12Device11* device, D3D12_ROOT_SIGNATURE_FLAGS flags) :
	m_Device(device),
	m_RootSignatureFlags(flags)
{}

void RootSignatureGenerator::AddParameter(D3D12_ROOT_PARAMETER rootParameter)
{
	m_RootParameters.emplace_back(rootParameter);
}

void RootSignatureGenerator::AddStaticSampler(D3D12_STATIC_SAMPLER_DESC staticSamplerDesc)
{
	m_StaticSamplers.emplace_back(staticSamplerDesc);
}

ComPtr<ID3D12RootSignature> RootSignatureGenerator::Generate()
{
	D3D12_ROOT_SIGNATURE_DESC RootSigDesc = {};
	RootSigDesc.NumParameters = (UINT)m_RootParameters.size();
	RootSigDesc.pParameters = m_RootParameters.empty() ? nullptr : m_RootParameters.data();
	RootSigDesc.NumStaticSamplers = (UINT)m_StaticSamplers.size();
	RootSigDesc.pStaticSamplers = m_StaticSamplers.empty() ? nullptr : m_StaticSamplers.data();
	RootSigDesc.Flags = m_RootSignatureFlags;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&RootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error));
	ThrowIfFailed(m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

	return m_rootSignature;
}
