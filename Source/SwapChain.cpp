#include "PCH.h"
#include "SwapChain.h"
#include "CommandQueue.h"

SwapChain::SwapChain()
{}

void SwapChain::Create(ID3D12Device11* device, CommandQueue* commandQueue, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle, HWND hWin, UINT width, UINT height, UINT bufferCount, BOOL vsync)
{
	m_CommandQueue = commandQueue;
	m_Device = device;
	m_SRVHandle = srvHandle;
	m_VSync = vsync;
	m_NumBuffers = bufferCount;

	ComPtr<IDXGIFactory7> dxgiFactory = GetDXGIFactory();

	HRESULT hr = dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &m_isTearingSupported, sizeof(m_isTearingSupported));
	if (FAILED(hr))
	{
		m_isTearingSupported = FALSE;
	}
	m_BufferWidth = width;
	m_BufferHeight = height;
	m_BackBuffers.resize(m_NumBuffers);

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = m_BufferWidth;
	swapChainDesc.Height = m_BufferHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = m_NumBuffers;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;  // was DXGI_SCALING_STRETCH
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = m_isTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullScreenDesc = {};
	swapChainFullScreenDesc.Windowed = true;

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(m_CommandQueue->GetPtr(), hWin, &swapChainDesc, &swapChainFullScreenDesc, nullptr, &swapChain1));

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen will be handled manually.
	ThrowIfFailed(dxgiFactory->MakeWindowAssociation(hWin, DXGI_MWA_NO_ALT_ENTER));
	ThrowIfFailed(swapChain1.As(&m_SwapChain));

	m_CurrentFrame = m_SwapChain->GetCurrentBackBufferIndex();
	m_RayTracingOutput.Create(m_Device, m_BufferWidth, m_BufferHeight, m_SRVHandle);

	for (UINT i = 0; i < m_NumBuffers; i++)
	{
		ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i])));
	}
}

void SwapChain::Present()
{
	const UINT syncInterval = m_VSync ? 1 : 0;
	const UINT presentFlags = m_isTearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));
	m_CurrentFrame = m_SwapChain->GetCurrentBackBufferIndex();
}

void SwapChain::Resize(UINT width, UINT height)
{
	if ((m_BufferWidth != width || m_BufferHeight != height) && (width != 0 && height != 0))
	{
		m_CommandQueue->Flush();
		m_BufferWidth = std::max(1u, width);
		m_BufferHeight = std::max(1u, height);
		for (UINT i = 0; i < m_NumBuffers; i++)
		{
			m_BackBuffers[i].Reset();
		}
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		DXGI_SWAP_CHAIN_DESC swapChainDesc2 = {};
		ThrowIfFailed(m_SwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(m_SwapChain->ResizeBuffers(m_NumBuffers, m_BufferWidth, m_BufferHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));
		ThrowIfFailed(m_SwapChain->GetDesc(&swapChainDesc2));
		D3D12_RESOURCE_DESC ResourceDesc1 = m_RayTracingOutput.GetResource()->GetDesc();
		m_CurrentFrame = m_SwapChain->GetCurrentBackBufferIndex();
		m_RayTracingOutput.Create(m_Device, m_BufferWidth, m_BufferHeight, m_SRVHandle);
		for (UINT i = 0; i < m_NumBuffers; i++)
		{
			ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i])));
		}
		D3D12_RESOURCE_DESC ResourceDesc2 = m_RayTracingOutput.GetResource()->GetDesc();
		UINT test = 0;
	}
}

void SwapChain::PrepareFrameStart(ID3D12GraphicsCommandList6* commandList)
{
	TransitionResource(commandList, m_BackBuffers[m_CurrentFrame].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
	m_RayTracingOutput.Transition(commandList, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

void SwapChain::PrepareFrameEnd(ID3D12GraphicsCommandList6* commandList)
{
	m_RayTracingOutput.Transition(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	commandList->CopyResource(m_BackBuffers[m_CurrentFrame].Get(), m_RayTracingOutput.GetResource());
	TransitionResource(commandList, m_BackBuffers[m_CurrentFrame].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
}
