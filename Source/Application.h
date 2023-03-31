#pragma once
#include "PCH.h"
#include "Window.h"
#include "Renderer.h"
#include "TopLevelASGenerator.h"

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
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRayGenSignature(ID3D12Device11* device);
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateMissSignature(ID3D12Device11* device);
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateHitSignature(ID3D12Device11* device);
	void CreateRaytracingPipeline(ID3D12Device11* device);
	Microsoft::WRL::ComPtr<IDxcBlob> m_rayGenLibrary;
	Microsoft::WRL::ComPtr<IDxcBlob> m_hitLibrary;
	Microsoft::WRL::ComPtr<IDxcBlob> m_missLibrary;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rayGenSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_hitSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_missSignature;

	void CreateDummyRootSignatures(ID3D12Device11* device);
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_DummyGlobalRootSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_DummyLocalRootSignature;
	// Ray tracing pipeline state
	Microsoft::WRL::ComPtr<ID3D12StateObject> m_rtStateObject;
	// Ray tracing pipeline state properties, retaining the shader identifiers
	// to use in the Shader Binding Table
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> m_rtStateObjectProps;
};
