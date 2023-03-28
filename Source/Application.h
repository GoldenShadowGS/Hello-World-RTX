#pragma once
#include "PCH.h"
#include "Window.h"
#include "Renderer.h"
#include "TopLevelASGenerator.h"

class Application
{
public:
	Application(HINSTANCE hInstance);
	~Application();
	int Run();
	void Update();
	void Render() ;
	void Exit() const;
private:
	void InitializeInput();
	void UpdateFrameTime();
	float GetFPS() const;
	void SetTitle() const;
	HINSTANCE m_hInstance;
	Window m_Window;
	Renderer m_Renderer;
	static Application* AppPtr;
	static LRESULT CALLBACK WindProcInit(HWND hwindow, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WindProc(HWND hwindow, UINT message, WPARAM wParam, LPARAM lParam);
	float m_FrameTime;
	WCHAR* m_TitleBuffer;
	// Assets
	void BuildAssets();
	struct AccelerationStructureBuffers
	{
		Microsoft::WRL::ComPtr<ID3D12Resource2> pScratch; // Scratch memory for AS builder 
		Microsoft::WRL::ComPtr<ID3D12Resource2> pResult; // Where the AS is 
		Microsoft::WRL::ComPtr<ID3D12Resource2> pInstanceDesc; // Hold the matrices of the instances
	};
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_bottomLevelAS; // Storage for the bottom Level AS
	nv_helpers_dx12::TopLevelASGenerator m_topLevelASGenerator;
	AccelerationStructureBuffers m_topLevelASBuffers;
	std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource2>, DirectX::XMMATRIX>> m_instances;
};
