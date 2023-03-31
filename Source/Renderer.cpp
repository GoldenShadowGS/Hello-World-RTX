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
	ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), m_pipelineState.Get()));


	//m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
	ID3D12DescriptorHeap* heaps = { m_DescriptorHeap.GetHeap() };

	//m_CommandList->SetDescriptorHeaps(1, &heaps);
	//m_CommandList->SetGraphicsRootDescriptorTable(0, m_DescriptorHeap.GetGPUHandle(0));

	TransitionResource(m_CommandList.Get(), m_SwapChain.GetBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_SwapChain.Clear(m_CommandList.Get());
}

void Renderer::Present()
{
	TransitionResource(m_CommandList.Get(), m_SwapChain.GetBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	ThrowIfFailed(m_CommandList->Close());
	m_CommandQueue.ExecuteCommandList(m_CommandList);
	m_SwapChain.Present();
	m_CommandQueue.Flush();
}

void Renderer::ExecuteCommandList()
{
	ThrowIfFailed(m_CommandList->Close());
	m_CommandQueue.ExecuteCommandList(m_CommandList);
	m_CommandQueue.Flush();
}

void Renderer::Resize()
{
	m_SwapChain.Resize();
}

void Renderer::BuildRootSignature()
{
	//Pixel Stage
	D3D12_DESCRIPTOR_RANGE1 texture2DRange = {};
	texture2DRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	texture2DRange.NumDescriptors = UINT_MAX; // UINT_MAX
	texture2DRange.BaseShaderRegister = 0;
	texture2DRange.RegisterSpace = 0;
	texture2DRange.OffsetInDescriptorsFromTableStart = 0;

	D3D12_ROOT_PARAMETER1 texture2DTable {};
	// Param 0 = Texture SRV Unbounded Table
	texture2DTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	texture2DTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; //D3D12_SHADER_VISIBILITY_ALL
	texture2DTable.DescriptorTable.NumDescriptorRanges = 1;
	texture2DTable.DescriptorTable.pDescriptorRanges = &texture2DRange;
	m_RootSignature.AddParameter(texture2DTable);

	D3D12_ROOT_PARAMETER1 rootParameter = {};

	// Param 1 = Texture ID
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter.Descriptor.ShaderRegister = 0; // TextureID
	rootParameter.Descriptor.RegisterSpace = 1;
	rootParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	m_RootSignature.AddParameter(rootParameter);

	// Param 2 = Pixel Shader Light Constant Buffer Descriptor
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter.Descriptor.ShaderRegister = 1; // Lights
	rootParameter.Descriptor.RegisterSpace = 1;
	rootParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	m_RootSignature.AddParameter(rootParameter);

	//Vertex Stage
	// Param 3 = Model Matrix
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter.Descriptor.ShaderRegister = 0; // Model Matrix
	rootParameter.Descriptor.RegisterSpace = 0;
	rootParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	m_RootSignature.AddParameter(rootParameter);

	// Param 4 = ViewProjection Matrix
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter.Descriptor.ShaderRegister = 1; // ViewProjection Matrix
	rootParameter.Descriptor.RegisterSpace = 0;
	rootParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	m_RootSignature.AddParameter(rootParameter);

	D3D12_STATIC_SAMPLER_DESC staticSamplerDesc = {};
	staticSamplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
	staticSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc.MipLODBias = 0.0f;
	staticSamplerDesc.MaxAnisotropy = 16;
	staticSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	staticSamplerDesc.MinLOD = 0;
	staticSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplerDesc.ShaderRegister = 0;
	staticSamplerDesc.RegisterSpace = 0;
	staticSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	m_RootSignature.AddStaticSampler(staticSamplerDesc);

	m_RootSignature.SetFlags(
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

	m_RootSignature.Build();
}

void Renderer::BuildPipeLineState()
{
	//------------
	// TODO Make this a function
	ComPtr<IDxcUtils> DXC_Utils;
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&DXC_Utils));
	ComPtr<IDxcIncludeHandler> DXC_IncludeHandler;
	DXC_Utils->CreateDefaultIncludeHandler(&DXC_IncludeHandler);
	ComPtr<IDxcLibrary> DXC_Library;
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&DXC_Library)));
	ComPtr<IDxcCompiler> DXC_Compiler;
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&DXC_Compiler)));
	ComPtr<IDxcBlob> IncludeBlob;
	DXC_IncludeHandler->LoadSource(L"Shaders/Shared.hlsli", IncludeBlob.GetAddressOf());

	const WCHAR* args = L"-I Shaders";

	D3D12_SHADER_BYTECODE VertexShaderByteCode = {};
	ComPtr<IDxcBlob> VScode = CompileShader(VertexShaderByteCode, L"Shaders/VertexShader.hlsl", L"main", L"vs_6_6", DXC_IncludeHandler.Get(), &args, 1, DXC_Library.Get(), DXC_Compiler.Get());

	D3D12_SHADER_BYTECODE PixelShaderByteCode = {};
	ComPtr<IDxcBlob> PScode = CompileShader(PixelShaderByteCode, L"Shaders/PixelShader.hlsl", L"main", L"ps_6_6", DXC_IncludeHandler.Get(), &args, 1, DXC_Library.Get(), DXC_Compiler.Get());

	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;

	D3D12_RENDER_TARGET_BLEND_DESC RenderTargetBlendDesc = {};
	RenderTargetBlendDesc.BlendEnable = FALSE;
	RenderTargetBlendDesc.LogicOpEnable = FALSE;
	RenderTargetBlendDesc.SrcBlend = D3D12_BLEND_ONE;
	RenderTargetBlendDesc.DestBlend = D3D12_BLEND_ZERO;
	RenderTargetBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	RenderTargetBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	RenderTargetBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	RenderTargetBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	RenderTargetBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	RenderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
	{
		blendDesc.RenderTarget[i] = RenderTargetBlendDesc;
	}

	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "MATERIALID", 0, DXGI_FORMAT_R32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;;
	rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;;
	rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.ForcedSampleCount = 0;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	D3D12_DEPTH_STENCIL_DESC DepthStencilDesc = {};
	DepthStencilDesc.DepthEnable = TRUE;
	DepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	DepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	DepthStencilDesc.StencilEnable = FALSE;
	DepthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;;
	DepthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	DepthStencilDesc.FrontFace = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	DepthStencilDesc.BackFace = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_RootSignature.Get();
	psoDesc.VS = VertexShaderByteCode;
	psoDesc.PS = PixelShaderByteCode;
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.BlendState = blendDesc;
	psoDesc.DepthStencilState = DepthStencilDesc;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	ThrowIfFailed(m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
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
	m_DescriptorHeap.Create(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	m_CommandAllocator = CreateCommandAllocator(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_CommandList = CreateCommandList(m_Device, m_CommandAllocator, nullptr, D3D12_COMMAND_LIST_TYPE_DIRECT);

	constexpr UINT64 UploadHeapSize = 1024ULL * 1024 * 500;
	constexpr UINT64 DefaultHeapSize = 1024ULL * 1024 * 500;
	constexpr UINT64 ConstantBufferHeapSize = 1024ULL * 1024 * 8;
	m_Heap.Create(m_Device.Get(), UploadHeapSize, DefaultHeapSize, ConstantBufferHeapSize);
}