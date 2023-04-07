#include "PCH.h"
#include "Renderer.h"
#include "Window.h"
#include "CommandQueue.h"
#include "SwapChain.h"
#include "DescriptorHeap.h"
#include "DX12Utility.h"

using Microsoft::WRL::ComPtr;

inline ComPtr<ID3D12Resource2> CreateDepthBuffer(const ComPtr<ID3D12Device11>& device, UINT width, UINT height)
{
	D3D12_HEAP_PROPERTIES HeapProperties = {};
	HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optimizedClearValue = {};
	optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	optimizedClearValue.DepthStencil = { 1.0f, 0 };

	ComPtr<ID3D12Resource2> Buffer;
	ThrowIfFailed(device->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimizedClearValue,
		IID_PPV_ARGS(&Buffer)));

	return Buffer;
}

SwapChain::SwapChain() :
	m_VSync(0),
	m_ViewPort { },
	m_ScissorRect { },
	m_Window(nullptr),
	m_CommandQueue(nullptr),
	m_Device(nullptr),
	m_ClearColor { 0.4f, 0.6f, 0.9f, 1.0f },
	m_BufferWidth(0),
	m_BufferHeight(0),
	m_NumBuffers(0),
	m_isTearingSupported(TRUE),
	m_CurrentFrame(0)
{}

void SwapChain::Create(ID3D12Device11* device, Window* window, CommandQueue* commandQueue, UINT bufferCount, BOOL vsync)
{
	m_Window = window;
	m_CommandQueue = commandQueue;
	m_Device = device;
	m_VSync = vsync;
	m_NumBuffers = bufferCount;

	ComPtr<IDXGIFactory7> dxgiFactory = GetDXGIFactory();

	HRESULT hr = dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &m_isTearingSupported, sizeof(m_isTearingSupported));
	if (FAILED(hr))
	{
		m_isTearingSupported = FALSE;
	}
	// Initialize Width and Height to the Client size of the window
	m_BufferWidth = m_Window->GetClientWidth();
	m_BufferHeight = m_Window->GetClientHeight();

	m_BackBuffers.resize(m_NumBuffers);

	// Initialize Descriptor Heap for RenderTargetViews
	//m_RenderTargetViewHeap.Create(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_NumBuffers, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	// Initialize Descriptor Heap for Depth Buffer
	//m_DepthStencilViewHeap.Create(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	// Initialize Depth Buffer Resource
	//m_DepthBuffer = CreateDepthBuffer(m_Device, m_BufferWidth, m_BufferHeight);

	// Initialize Render Target Buffer Resources
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
	ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(m_CommandQueue->GetPtr(), m_Window->GetHandle(), &swapChainDesc, &swapChainFullScreenDesc, nullptr, &swapChain1));

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen will be handled manually.
	ThrowIfFailed(dxgiFactory->MakeWindowAssociation(m_Window->GetHandle(), DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain1.As(&m_SwapChain));

	m_CurrentFrame = m_SwapChain->GetCurrentBackBufferIndex();

	UpdateBackBuffers();
	SetViewPort(m_BufferWidth, m_BufferHeight);
	SetScissorRect(m_BufferWidth, m_BufferHeight);
}

void SwapChain::Present()
{
	const UINT syncInterval = m_VSync ? 1 : 0;
	const UINT presentFlags = m_isTearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));
	m_CurrentFrame = m_SwapChain->GetCurrentBackBufferIndex();
}

void SwapChain::CopyResourceToBackBuffer(ID3D12GraphicsCommandList6* commandList, ID3D12Resource2* resource)
{
	commandList->CopyResource(m_BackBuffers[m_CurrentFrame].Get(), resource);
}

//void SwapChain::Clear(ID3D12GraphicsCommandList6* commandList)
//{
//	const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRTVCPUDescriptorHandle();
//	const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDSVCPUDescriptorHandle();
//	commandList->RSSetViewports(1, &m_ViewPort);
//	commandList->RSSetScissorRects(1, &m_ScissorRect);
//	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
//	commandList->ClearRenderTargetView(rtvHandle, m_ClearColor, 0, nullptr);
//	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
//}

bool SwapChain::Resize()
{
	UINT newWidth = m_Window->GetClientWidth();
	UINT newHeight = m_Window->GetClientHeight();
	if ((m_BufferWidth != newWidth || m_BufferHeight != newHeight) && !(newHeight == 0 && newWidth == 0))
	{
		// Flush the GPU queue to make sure the swap chain's back buffers are not being referenced by an in-flight command list.
		m_CommandQueue->Flush();

		m_BufferWidth = std::max(1u, newWidth);
		m_BufferHeight = std::max(1u, newHeight);

		SetViewPort(m_BufferWidth, m_BufferHeight);
		SetScissorRect(m_BufferWidth, m_BufferHeight);

		// Reset the ComPtrs
		for (UINT i = 0; i < m_NumBuffers; i++)
		{
			m_BackBuffers[i].Reset();
		}
		//m_DepthBuffer.Reset();

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(m_SwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(m_SwapChain->ResizeBuffers(m_NumBuffers, m_BufferWidth, m_BufferHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));
		m_CurrentFrame = m_SwapChain->GetCurrentBackBufferIndex();

		// Resize Depth Buffer
		//m_DepthBuffer = CreateDepthBuffer(m_Device, m_BufferWidth, m_BufferHeight);

		UpdateBackBuffers();
		return true;
	}
	return false;
}

void SwapChain::SetViewPortScissorRect(ID3D12GraphicsCommandList6* commandList)
{
	commandList->RSSetViewports(1, &m_ViewPort);
	commandList->RSSetScissorRects(1, &m_ScissorRect);
}

void SwapChain::TransitionBackBuffer(ID3D12GraphicsCommandList6* commandList, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
	TransitionResource(commandList, m_BackBuffers[m_CurrentFrame].Get(), beforeState, afterState);
}

void SwapChain::UpdateBackBuffers()
{
	for (UINT i = 0; i < m_NumBuffers; i++)
	{
		ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i])));
		//m_Device->CreateRenderTargetView(m_BackBuffers[i].Get(), nullptr, m_RenderTargetViewHeap.GetCPUHandle(i));
	}
	//D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
	//dsv.Format = DXGI_FORMAT_D32_FLOAT;
	//dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	//dsv.Texture2D.MipSlice = 0;
	//dsv.Flags = D3D12_DSV_FLAG_NONE;
	//m_Device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsv, m_DepthStencilViewHeap.GetCPUHandle(0));
}

void SwapChain::SetViewPort(UINT newWidth, UINT newHeight)
{
	m_ViewPort.TopLeftX = 0.0f;
	m_ViewPort.TopLeftY = 0.0f;
	m_ViewPort.Width = (FLOAT)newWidth;
	m_ViewPort.Height = (FLOAT)newHeight;
	m_ViewPort.MinDepth = D3D12_MIN_DEPTH;
	m_ViewPort.MaxDepth = D3D12_MAX_DEPTH;
}

void SwapChain::SetScissorRect(UINT newWidth, UINT newHeight)
{
	m_ScissorRect.left = 0;
	m_ScissorRect.top = 0;
	m_ScissorRect.right = newWidth;
	m_ScissorRect.bottom = newHeight;
}
