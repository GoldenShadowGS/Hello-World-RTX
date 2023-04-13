#pragma once
#include "PCH.h"
#include "Window.h"
#include "Renderer.h"
#include "Camera.h"
#include "AccelerationStructure.h"

class HeapManager;

class Application
{
public:
	Application(HINSTANCE hInstance);
	~Application();
	int Run();
	void Resize();
	void Update();
	void Render();
	void Exit() const;
private:
	void InitializeInput();
	void UpdateFrameTime();
	float GetFPS() const;
	void SetTitle() const;
	void OnInit();
	void BuildAssets(ID3D12GraphicsCommandList6* commandList);
	void BuildScene(ID3D12GraphicsCommandList6* commandList);
	void CreateRaytracingPipeline(ID3D12Device11* device);
	void CreateRootSignatures(ID3D12Device11* device);
	void CreateShaderBindingTable(ID3D12Device11* device, DescriptorHeap* descriptorHeap);
	static LRESULT CALLBACK WindProcInit(HWND hwindow, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WindProc(HWND hwindow, UINT message, WPARAM wParam, LPARAM lParam);
	static Application* AppPtr;
	HINSTANCE m_hInstance;
	Window m_Window;
	Renderer m_Renderer;
	float m_FrameTime;
	WCHAR* m_TitleBuffer;
	SceneAccelerationStructure m_Scene;
	Microsoft::WRL::ComPtr<IDxcBlob> m_rayGenLibrary;
	Microsoft::WRL::ComPtr<IDxcBlob> m_hitLibrary;
	Microsoft::WRL::ComPtr<IDxcBlob> m_missLibrary;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rayGenSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_hitSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_missSignature;
	Microsoft::WRL::ComPtr<ID3D12StateObject> m_StateObject;
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_ShaderBindingTable;
	Camera m_Camera;
	StructuredBuffer m_StructuredBuffer;
	float angle1 = 0.0f;
	float angle2 = 0.0f;
};
