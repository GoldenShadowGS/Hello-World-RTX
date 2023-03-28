#include "PCH.h"

class RootSignature
{
public:
	RootSignature();
	void Create(ID3D12Device11* device);
	void AddParameter(D3D12_ROOT_PARAMETER1 rootParameter);
	void AddStaticSampler(D3D12_STATIC_SAMPLER_DESC staticSamplerDesc);
	inline void SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags) { m_RootSignatureFlags = flags; }
	void Build();
	inline ID3D12RootSignature* Get() { return m_rootSignature.Get(); }
private:
	ID3D12Device11* m_Device;
	D3D_ROOT_SIGNATURE_VERSION GetRootSignatureVersion(ID3D12Device11* device);
	D3D_ROOT_SIGNATURE_VERSION m_Version;
	D3D12_ROOT_SIGNATURE_FLAGS m_RootSignatureFlags;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	std::vector<D3D12_ROOT_PARAMETER> m_RootParameters1_0;
	std::vector<D3D12_ROOT_PARAMETER1> m_RootParameters1_1;
	std::vector<D3D12_STATIC_SAMPLER_DESC> m_StaticSamplers;
};