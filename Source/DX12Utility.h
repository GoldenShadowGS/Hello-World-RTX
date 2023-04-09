#pragma once
#include "PCH.h"

using Microsoft::WRL::ComPtr;

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

inline void EnableDX12DebugLayer()
{
#if defined(_DEBUG)
	ComPtr<ID3D12Debug6> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
	debugInterface->SetEnableAutoName(true);
	debugInterface->SetEnableGPUBasedValidation(true);
	OutputDebugStringW(L"D3D12 Debug Layer enabled\n");
#endif
}

inline std::wstring GetAdapterDescription(const ComPtr<IDXGIAdapter4>& dxgiAdapter)
{
	DXGI_ADAPTER_DESC3 dxgiAdapterDesc;
	dxgiAdapter->GetDesc3(&dxgiAdapterDesc);
	std::wstring desc = dxgiAdapterDesc.Description;
	std::wstring result = L"------------------------\nAdapter: " + desc + L"\n" + L"Video Memory: " + std::to_wstring(dxgiAdapterDesc.DedicatedVideoMemory) + L"\n";
	return result;
}

inline std::wstring GetAdapterName(const ComPtr<IDXGIAdapter4>& dxgiAdapter)
{
	DXGI_ADAPTER_DESC3 dxgiAdapterDesc;
	dxgiAdapter->GetDesc3(&dxgiAdapterDesc);
	std::wstring result = dxgiAdapterDesc.Description;
	return result;
}

inline void CheckShaderModel6_6Support(const ComPtr<ID3D12Device11>& device)
{
	D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_6 };
	HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

inline ComPtr<IDXGIFactory7> GetDXGIFactory()
{
	ComPtr<IDXGIFactory7> dxgiFactory;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
	return dxgiFactory;
}

inline ComPtr<IDXGIAdapter4> GetAdapter()
{
	ComPtr<IDXGIFactory7> dxgiFactory = GetDXGIFactory();

	ComPtr<IDXGIAdapter4> dxgiAdapter;
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&dxgiAdapter)) != DXGI_ERROR_NOT_FOUND; i++)
	{
		HRESULT hr = D3D12CreateDevice(dxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_2, __uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr))
		{
#ifdef _DEBUG
			BOOL tearingSupport = TRUE;
			HRESULT hr = dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearingSupport, sizeof(tearingSupport));
			if (FAILED(hr))
			{
				tearingSupport = FALSE;
			}
			OutputDebugStringW(GetAdapterDescription(dxgiAdapter).c_str());
			std::wstring sp = tearingSupport ? L"Yes" : L"No";
			OutputDebugStringW(L"Tearing Support ");
			OutputDebugStringW(sp.c_str());
			OutputDebugStringW(L"\n------------------------\n");
#endif
			return dxgiAdapter;
		}
	}
	// If no adapter found, throw an error
	throw std::exception();
}

inline void CheckRaytracingSupport(ID3D12Device11* device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
	ThrowIfFailed(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)));
	if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
		throw std::runtime_error("Raytracing not supported on device"); // TODO error message?
}

inline void TransitionResource(ID3D12GraphicsCommandList6* commandList, ID3D12Resource2* resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = beforeState;
	barrier.Transition.StateAfter = afterState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	commandList->ResourceBarrier(1, &barrier);
}

inline UINT Align(UINT uLocation, UINT uAlign)
{
	assert(!((0 == uAlign) || (uAlign & (uAlign - 1))));
	return ((uLocation + (uAlign - 1)) & ~(uAlign - 1));
}

inline UINT64 Align64(UINT64 uLocation, UINT64 uAlign)
{
	assert(!((0 == uAlign) || (uAlign & (uAlign - 1))));
	return ((uLocation + (uAlign - 1)) & ~(uAlign - 1));
}

//--------------------------------------------------------------------------------------------------
// Compile a HLSL file into a DXIL library
//
inline IDxcBlob* CompileShaderLibrary(LPCWSTR fileName)
{
	static IDxcCompiler* pCompiler = nullptr;
	static IDxcLibrary* pLibrary = nullptr;
	static IDxcIncludeHandler* dxcIncludeHandler;

	HRESULT hr;

	// Initialize the DXC compiler and compiler helper
	if (!pCompiler)
	{
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), (void**)&pCompiler));
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void**)&pLibrary));
		ThrowIfFailed(pLibrary->CreateIncludeHandler(&dxcIncludeHandler));
	}
	// Open and read the file
	std::ifstream shaderFile(fileName);
	if (shaderFile.good() == false)
	{
		CStringA errmsg = "Cannot find shader file: ";
		errmsg.Append(CStringA(fileName));
		throw std::logic_error(errmsg);
	}
	std::stringstream strStream;
	strStream << shaderFile.rdbuf();
	std::string sShader = strStream.str();

	// Create blob from the string
	IDxcBlobEncoding* pTextBlob;
	ThrowIfFailed(pLibrary->CreateBlobWithEncodingFromPinned(
		(LPBYTE)sShader.c_str(), (uint32_t)sShader.size(), 0, &pTextBlob));

	// Compile
	IDxcOperationResult* pResult;
	ThrowIfFailed(pCompiler->Compile(pTextBlob, fileName, L"", L"lib_6_3", nullptr, 0, nullptr, 0,
		dxcIncludeHandler, &pResult));

	// Verify the result
	HRESULT resultCode;
	ThrowIfFailed(pResult->GetStatus(&resultCode));
	if (FAILED(resultCode))
	{
		IDxcBlobEncoding* pError;
		hr = pResult->GetErrorBuffer(&pError);
		if (FAILED(hr))
		{
			throw std::logic_error("Failed to get shader compiler error");
		}
		// Convert error blob to a string
		std::vector<char> infoLog(pError->GetBufferSize() + 1);
		memcpy(infoLog.data(), pError->GetBufferPointer(), pError->GetBufferSize());
		infoLog[pError->GetBufferSize()] = 0;

		std::string errorMsg = "Shader Compiler Error:\n";
		errorMsg.append(infoLog.data());

		throw std::logic_error(errorMsg.c_str());
	}
#ifdef _DEBUG
	{
		IDxcBlobEncoding* pError;
		hr = pResult->GetErrorBuffer(&pError);
		if (FAILED(hr))
		{
			throw std::logic_error("Failed to get shader compiler error");
		}
		if (pError->GetBufferSize())
		{
			// Convert error blob to a string
			std::vector<char> infoLog(pError->GetBufferSize() + 1);
			memcpy(infoLog.data(), pError->GetBufferPointer(), pError->GetBufferSize());
			infoLog[pError->GetBufferSize()] = 0;

			std::string errorMsg = "\n";
			errorMsg.append(infoLog.data());

			OutputDebugStringA(errorMsg.c_str());
		}
	}
#endif

	IDxcBlob* pBlob;
	ThrowIfFailed(pResult->GetResult(&pBlob));
	return pBlob;
}