#pragma once
#include "PCH.h"
#include "DX12Utility.h"
#include "OutputBuffer.h"

class CommandQueue;

class SwapChain
{
public:
	SwapChain();
	void Create(ID3D12Device11* device, CommandQueue* commandQueue, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle, HWND hWin, UINT width, UINT height, UINT bufferCount, BOOL vsync);
	void Present();
	void Resize(UINT width, UINT height);
	void PrepareFrameStart(ID3D12GraphicsCommandList6* commandList);
	void PrepareFrameEnd(ID3D12GraphicsCommandList6* commandList);
	UINT GetWidth() const { return (UINT)m_BufferWidth; }
	UINT GetHeight() const { return (UINT)m_BufferHeight; }
	inline float GetAspectRatio() const { return (float)m_BufferWidth / (float)m_BufferHeight; };
	BOOL m_VSync;
private:
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource2>> m_BackBuffers;
	OutputBuffer m_RayTracingOutput;
	CommandQueue* m_CommandQueue = nullptr;
	ID3D12Device11* m_Device = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHandle = {};
	const float m_ClearColor[4] = {};
	UINT m_BufferWidth = 0;
	UINT m_BufferHeight = 0;
	UINT m_NumBuffers= 0;
	BOOL m_isTearingSupported = TRUE;
	UINT m_CurrentFrame = 0;
};