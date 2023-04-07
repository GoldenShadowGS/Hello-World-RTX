#pragma once
#include "PCH.h"
#include "DescriptorHeap.h"

class CommandQueue;
class Window;

class SwapChain
{
public:
	SwapChain();
	void Create(ID3D12Device11* device, Window* window, CommandQueue* commandQueue, UINT bufferCount, BOOL vsync);
	void Present();
	//void Clear(ID3D12GraphicsCommandList6* commandList);
	bool Resize();
	void SetViewPortScissorRect(ID3D12GraphicsCommandList6* commandList);
	void TransitionBackBuffer(ID3D12GraphicsCommandList6* commandList, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
	void CopyResourceToBackBuffer(ID3D12GraphicsCommandList6* commandList, ID3D12Resource2* resource);
	inline ID3D12Resource2* GetBackBuffer() { return m_BackBuffers[m_CurrentFrame].Get(); }
	//inline D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUDescriptorHandle() { return m_RenderTargetViewHeap.GetCPUHandle(m_CurrentFrame); }
	//inline D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUDescriptorHandle() { return m_DepthStencilViewHeap.GetCPUHandle(0); }
private:
	void UpdateBackBuffers();
	void SetViewPort(UINT newWidth, UINT newHeight);
	void SetScissorRect(UINT newWidth, UINT newHeight);
public:
	BOOL m_VSync;
	D3D12_VIEWPORT m_ViewPort;
	D3D12_RECT m_ScissorRect;
private:
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource2>> m_BackBuffers;
	//DescriptorHeap m_RenderTargetViewHeap;

	//Microsoft::WRL::ComPtr<ID3D12Resource2> m_DepthBuffer;
	//DescriptorHeap m_DepthStencilViewHeap;

	Window* m_Window;
	CommandQueue* m_CommandQueue;
	ID3D12Device11* m_Device;
	const float m_ClearColor[4];
	UINT m_BufferWidth;
	UINT m_BufferHeight;
	UINT m_NumBuffers;
	BOOL m_isTearingSupported;
	UINT m_CurrentFrame;
public:
	inline float GetAspectRatio() const { return (float)m_BufferWidth / (float)m_BufferHeight; };
};