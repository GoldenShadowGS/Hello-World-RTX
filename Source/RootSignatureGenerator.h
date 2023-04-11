#include "PCH.h"

class RootSignatureGenerator
{
public:
	RootSignatureGenerator(ID3D12Device11* device, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
	void AddParameter(D3D12_ROOT_PARAMETER rootParameter);
	void AddStaticSampler(D3D12_STATIC_SAMPLER_DESC staticSamplerDesc);
	Microsoft::WRL::ComPtr<ID3D12RootSignature> Generate();
private:
	ID3D12Device11* m_Device;
	D3D12_ROOT_SIGNATURE_FLAGS m_RootSignatureFlags;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	std::vector<D3D12_ROOT_PARAMETER> m_RootParameters;
	std::vector<D3D12_STATIC_SAMPLER_DESC> m_StaticSamplers;
};