#include "PCH.h"
#include "RootSignature.h"

RootSignature::RootSignature() : m_Device(nullptr), m_Version {}, m_RootSignatureFlags {}
{}

void RootSignature::Create(ID3D12Device11* device)
{
	m_Device = device;
	m_Version = GetRootSignatureVersion(device);
}

void RootSignature::AddParameter(D3D12_ROOT_PARAMETER1 rootParameter)
{
	if (m_Version == D3D_ROOT_SIGNATURE_VERSION_1_0)
	{
		D3D12_ROOT_PARAMETER rootParameter1_0 = {};
		rootParameter1_0.ParameterType = rootParameter.ParameterType;
		rootParameter1_0.ShaderVisibility = rootParameter.ShaderVisibility;
		D3D12_DESCRIPTOR_RANGE* range1_0 = nullptr;
		switch (rootParameter.ParameterType)
		{
		case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
			range1_0 = (D3D12_DESCRIPTOR_RANGE*)rootParameter.DescriptorTable.pDescriptorRanges;
			range1_0->OffsetInDescriptorsFromTableStart = rootParameter.DescriptorTable.pDescriptorRanges->OffsetInDescriptorsFromTableStart;
			rootParameter1_0.DescriptorTable.NumDescriptorRanges = rootParameter.DescriptorTable.NumDescriptorRanges;
			rootParameter1_0.DescriptorTable.pDescriptorRanges = (D3D12_DESCRIPTOR_RANGE*)rootParameter.DescriptorTable.pDescriptorRanges;
			break;
		case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
			rootParameter1_0.Constants.RegisterSpace = rootParameter.Constants.RegisterSpace;
			rootParameter1_0.Constants.ShaderRegister = rootParameter.Constants.ShaderRegister;
			rootParameter1_0.Constants.Num32BitValues = rootParameter.Constants.Num32BitValues;
			break;
		default:
			rootParameter1_0.Descriptor.ShaderRegister = rootParameter.Descriptor.ShaderRegister;
			rootParameter1_0.Descriptor.RegisterSpace = rootParameter.Descriptor.RegisterSpace;
			break;
		}
		m_RootParameters1_0.emplace_back(rootParameter1_0);
	}
	else
	{
		m_RootParameters1_1.emplace_back(rootParameter);
	}
}

void RootSignature::AddStaticSampler(D3D12_STATIC_SAMPLER_DESC staticSamplerDesc)
{
	m_StaticSamplers.emplace_back(staticSamplerDesc);
}

void RootSignature::Build()
{
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC RootSigDesc = {};
	RootSigDesc.Version = m_Version;
	if (m_Version == D3D_ROOT_SIGNATURE_VERSION_1_0)
	{
		RootSigDesc.Desc_1_0.NumParameters = (UINT)m_RootParameters1_0.size();
		RootSigDesc.Desc_1_0.pParameters = m_RootParameters1_0.empty() ? nullptr : m_RootParameters1_0.data();
		RootSigDesc.Desc_1_0.NumStaticSamplers = (UINT)m_StaticSamplers.size();
		RootSigDesc.Desc_1_0.pStaticSamplers = m_StaticSamplers.empty() ? nullptr : m_StaticSamplers.data();
		RootSigDesc.Desc_1_0.Flags = m_RootSignatureFlags;
	}
	else
	{
		RootSigDesc.Desc_1_1.NumParameters = (UINT)m_RootParameters1_1.size();
		RootSigDesc.Desc_1_1.pParameters = m_RootParameters1_1.empty() ? nullptr : m_RootParameters1_1.data();
		RootSigDesc.Desc_1_1.NumStaticSamplers = (UINT)m_StaticSamplers.size();
		RootSigDesc.Desc_1_1.pStaticSamplers = m_StaticSamplers.empty() ? nullptr : m_StaticSamplers.data();
		RootSigDesc.Desc_1_1.Flags = m_RootSignatureFlags;
	}
	using Microsoft::WRL::ComPtr;
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeVersionedRootSignature(&RootSigDesc, &signature, &error));
	ThrowIfFailed(m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

D3D_ROOT_SIGNATURE_VERSION RootSignature::GetRootSignatureVersion(ID3D12Device11* device)
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		return D3D_ROOT_SIGNATURE_VERSION_1_0;
	}
	return D3D_ROOT_SIGNATURE_VERSION_1_1;
}