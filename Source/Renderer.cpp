#include "PCH.h"
#include "Renderer.h"
#include "Window.h"
#include "DX12Utility.h"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 608; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

using Microsoft::WRL::ComPtr;

// Local Static functions

ComPtr<ID3D12Device11> CreateDevice(IDXGIAdapter4* adapter)
{
	ComPtr<ID3D12Device11> d3d12Device;
	ThrowIfFailed(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&d3d12Device)));
	CheckShaderModel6_6Support(d3d12Device);
	CheckRaytracingSupport(d3d12Device.Get());
#if defined(_DEBUG)
	// Enable debug messages in debug mode.
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(d3d12Device.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif
	return d3d12Device;
}

ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ID3D12Device11* device, D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList6> CreateCommandList(
	ID3D12Device11* device,
	ID3D12CommandAllocator* commandAllocator,
	D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12GraphicsCommandList6> commandList;
	ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator, nullptr, IID_PPV_ARGS(&commandList)));
	return commandList;
}

//----------------------------------------
// Renderer
//----------------------------------------

Renderer::Renderer()
{}

Renderer::~Renderer()
{
	if (m_Device)
	{
		m_CommandQueue.Flush();
#ifdef _DEBUG
		{
			ComPtr<ID3D12InfoQueue> InfoQueue;
			ThrowIfFailed(m_Device->QueryInterface(IID_PPV_ARGS(&InfoQueue)));
			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
		}
		//Create Debug Device right before all of the ComPtrs go out of scope
		ThrowIfFailed(m_Device->QueryInterface(IID_PPV_ARGS(&m_DebugWrapper.m_DebugDevice)));
#endif
	}
}

void Renderer::Create(Window* window)
{
	EnableDX12DebugLayer();
	ComPtr<IDXGIAdapter4> dxgiAdapter = GetAdapter();
	m_Device = CreateDevice(dxgiAdapter.Get());
#ifdef _DEBUG
	m_Device->SetName(GetAdapterName(dxgiAdapter).c_str());
#endif

	m_CommandQueue.Create(m_Device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_SwapChain.Create(m_Device.Get(), window, &m_CommandQueue, 3, TRUE);
	m_DescriptorHeap.Create(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 10, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	m_CommandAllocator = CreateCommandAllocator(m_Device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_CommandList = CreateCommandList(m_Device.Get(), m_CommandAllocator.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);

	constexpr UINT64 UploadHeapSize = 1024ULL * 1024 * 20;
	constexpr UINT64 ScratchUploadHeapSize = 1024ULL * 1024 * 20;
	constexpr UINT64 DefaultHeapSize = 1024ULL * 1024 * 20;
	constexpr UINT64 ScratchDefaultHeapSize = 1024ULL * 1024 * 20;
	m_Heap.Create(m_Device.Get(), UploadHeapSize, ScratchUploadHeapSize, DefaultHeapSize, ScratchDefaultHeapSize);
}

void Renderer::ExecuteCommandList()
{
	ThrowIfFailed(m_CommandList->Close());
	m_CommandQueue.ExecuteCommandList(m_CommandList.Get());
	m_CommandQueue.Flush();
}

#ifdef _DEBUG
Renderer::DebugWrapper::~DebugWrapper()
{
	if (m_DebugDevice)
	{
		//Report Ref Count: Should only contain a single ref to this Debug Device.
		ThrowIfFailed(m_DebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));
	}
}
#endif