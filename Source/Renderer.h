#pragma once
#include "PCH.h"
#include "SwapChain.h"
#include "CommandQueue.h"
#include "DescriptorHeap.h"
#include "RootSignature.h"
#include "Heap.h"

class Window;

class Renderer
{
public:
	Renderer();
	~Renderer();
	void Create(Window* m_Window);
	void StartNextFrame();
	void Present();
	void ExecuteCopyToGPU();
	void Resize();
	inline void ToggleVSync() { SetVSync(!GetVSync()); }
	inline void SetVSync(bool vsync) { m_SwapChain.m_VSync = vsync; }
	inline bool GetVSync() const { return m_SwapChain.m_VSync; }
	inline ID3D12Device11* GetDevice() { return m_Device.Get(); }
	inline ID3D12GraphicsCommandList6* GetCommandList() { return m_CommandList.Get(); }
	inline DescriptorHeap* GetDescriptorHeap() { return &m_DescriptorHeap; }
	inline HeapManager* GetHeap() { return &m_Heap; }
private:
	void BuildRootSignature();
	void BuildPipeLineState();
#ifdef _DEBUG
	//Gets Destructed Last Because it was created First
	class DebugWrapper
	{
	public:
		~DebugWrapper();
		Microsoft::WRL::ComPtr<ID3D12DebugDevice2> m_DebugDevice;
	} m_DebugWrapper;
#endif
	// DirectX 12 Objects
	Microsoft::WRL::ComPtr<ID3D12Device11> m_Device;
	CommandQueue m_CommandQueue;
	SwapChain m_SwapChain;
	RootSignature m_RootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	DescriptorHeap m_DescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> m_CommandList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
	HeapManager m_Heap;
	friend class Camera;
	friend class SwapChain;
};