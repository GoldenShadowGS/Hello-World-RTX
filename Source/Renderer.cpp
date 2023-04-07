#include "PCH.h"
#include "Renderer.h"
#include "Window.h"
#include "DX12Utility.h"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 608; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

using Microsoft::WRL::ComPtr;

// Local Static functions

inline ComPtr<ID3D12Device11> CreateDevice(const ComPtr<IDXGIAdapter4>& adapter)
{
	ComPtr<ID3D12Device11> d3d12Device;
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&d3d12Device)));
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

inline ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(const ComPtr<ID3D12Device11>& device, D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

inline ComPtr<ID3D12GraphicsCommandList6> CreateCommandList(const ComPtr<ID3D12Device11>& device,
	const ComPtr<ID3D12CommandAllocator>& commandAllocator, const ComPtr<ID3D12PipelineState>& pipelineState, D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12GraphicsCommandList6> commandList;
	ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), pipelineState.Get(), IID_PPV_ARGS(&commandList)));
	return commandList;
}

//----------------------------------------
// Renderer
//----------------------------------------

void Renderer::StartNextFrame()
{
	ThrowIfFailed(m_CommandAllocator.Get()->Reset());
	ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));

	m_SwapChain.TransitionBackBuffer(m_CommandList.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);

	//m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
	//ID3D12DescriptorHeap* heaps = { m_DescriptorHeap.GetHeap() };

	//m_CommandList->SetDescriptorHeaps(1, &heaps);
	//m_CommandList->SetGraphicsRootDescriptorTable(0, m_DescriptorHeap.GetGPUHandle(0));
	//TransitionResource(m_CommandList.Get(), m_SwapChain.GetBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	//m_SwapChain.Clear(m_CommandList.Get());
}

void Renderer::Present()
{
	m_SwapChain.TransitionBackBuffer(m_CommandList.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
	//TransitionResource(m_CommandList.Get(), m_SwapChain.GetBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	ThrowIfFailed(m_CommandList->Close());
	m_CommandQueue.ExecuteCommandList(m_CommandList.Get());
	m_SwapChain.Present();
	m_CommandQueue.Flush();
}

void Renderer::ExecuteCommandList()
{
	ThrowIfFailed(m_CommandList->Close());
	m_CommandQueue.ExecuteCommandList(m_CommandList.Get());
	m_CommandQueue.Flush();
}

Renderer::Renderer()
{}

Renderer::~Renderer()
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

#ifdef _DEBUG
Renderer::DebugWrapper::~DebugWrapper()
{
	//Report Ref Count: Should only contain a single ref to this Debug Device.
	ThrowIfFailed(m_DebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));
}
#endif

void Renderer::Create(Window* window)
{
	EnableDX12DebugLayer();
	ComPtr<IDXGIAdapter4> dxgiAdapter = GetAdapter();
	m_Device = CreateDevice(dxgiAdapter);
#ifdef _DEBUG
	m_Device->SetName(GetAdapterName(dxgiAdapter).c_str());
#endif

	m_CommandQueue.Create(m_Device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);

	m_SwapChain.Create(m_Device.Get(), window, &m_CommandQueue, 2, TRUE);

	//m_RootSignature.Create(m_Device.Get());
	//BuildRootSignature();
	//BuildPipeLineState();

	m_DescriptorHeap.Create(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	m_CommandAllocator = CreateCommandAllocator(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_CommandList = CreateCommandList(m_Device, m_CommandAllocator, nullptr, D3D12_COMMAND_LIST_TYPE_DIRECT);

	constexpr UINT64 UploadHeapSize = 1024ULL * 1024 * 500;
	constexpr UINT64 DefaultHeapSize = 1024ULL * 1024 * 500;
	constexpr UINT64 ConstantBufferHeapSize = 1024ULL * 1024 * 8;
	m_Heap.Create(m_Device.Get(), UploadHeapSize, DefaultHeapSize, ConstantBufferHeapSize);
}