#pragma once
#include "PCH.h"
#include "Window.h"
#include "Renderer.h"
#include "Camera.h"

struct Vertex
{
	float x, y, z;
};

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
	HINSTANCE m_hInstance;
	Window m_Window;
	Renderer m_Renderer;
	static Application* AppPtr;
	static LRESULT CALLBACK WindProcInit(HWND hwindow, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WindProc(HWND hwindow, UINT message, WPARAM wParam, LPARAM lParam);
	float m_FrameTime;
	WCHAR* m_TitleBuffer;

	void OnInit();
	// Assets
	void BuildAssets(ID3D12Device11* device, ID3D12GraphicsCommandList6* commandList);

	Microsoft::WRL::ComPtr<ID3D12Resource2> Matrix;			// Holds the matrices of the instances
	Microsoft::WRL::ComPtr<ID3D12Resource2> VertexBuffer;	// Holds the Vertices
	Microsoft::WRL::ComPtr<ID3D12Resource2> IndexBuffer;	// Holds the indices
	Microsoft::WRL::ComPtr<ID3D12Resource2> BLScratch;		// Scratch memory for AS builder 
	Microsoft::WRL::ComPtr<ID3D12Resource2> BLASResult;		// Where the AS is 

	Microsoft::WRL::ComPtr<ID3D12Resource2> TLScratch;		// Scratch memory for AS builder 
	Microsoft::WRL::ComPtr<ID3D12Resource2> TLASResult;		// Where the AS is 
	Microsoft::WRL::ComPtr<ID3D12Resource2> descriptorsBuffer;

	// #DXR
	void CreateRaytracingPipeline(ID3D12Device11* device);
	Microsoft::WRL::ComPtr<IDxcBlob> m_rayGenLibrary;
	Microsoft::WRL::ComPtr<IDxcBlob> m_hitLibrary;
	Microsoft::WRL::ComPtr<IDxcBlob> m_missLibrary;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rayGenSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_hitSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_missSignature;

	void CreateRootSignatures(ID3D12Device11* device);
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_GlobalRootSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_LocalRootSignature;
	// Ray tracing pipeline state
	Microsoft::WRL::ComPtr<ID3D12StateObject> m_rtStateObject;
	// Ray tracing pipeline state properties, retaining the shader identifiers
	// to use in the Shader Binding Table
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> m_rtStateObjectProps;

	Microsoft::WRL::ComPtr<ID3D12Resource2> m_outputResource;

	void CreateRaytracingOutputBuffer();
	void CreateShaderResourceHeap(ID3D12Device11* device, DescriptorHeap* descriptorHeap);

	void CreateShaderBindingTable(ID3D12Device11* device, DescriptorHeap* descriptorHeap);
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_ShaderBindingTable;

	const WCHAR* RayGenName = L"RayGen";
	const WCHAR* MissName = L"Miss";
	const WCHAR* ClosestHitName = L"ClosestHit";
	const WCHAR* HitGroupName = L"HitGroup";
	Camera m_Camera;
};
