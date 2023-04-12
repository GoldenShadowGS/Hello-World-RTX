#include "PCH.h"
#include "Application.h"
#include "Input.h"
#include "DX12Utility.h"
#include "RootSignatureGenerator.h"
#include "Heap.h"
#include "PipelineStateObject.h"

#define WINDOWTITLE L"Hello World RTX"
#define FULLSCREENMODE false
#define TITLE_BUFFER_SIZE 256

using namespace DirectX;

Application* Application::AppPtr = nullptr;

Application::Application(HINSTANCE hInstance) : m_hInstance(hInstance), m_FrameTime(0.0f), m_TitleBuffer(nullptr)
{}

Application::~Application()
{
	if (m_TitleBuffer)
		delete[] m_TitleBuffer; //TODO Temporary to show stats in title bar
}

int Application::Run()
{
	m_TitleBuffer = new WCHAR[TITLE_BUFFER_SIZE];  //TODO Temporary to show stats in title bar
	try
	{
		m_Window.Create(m_hInstance, this, WINDOWTITLE, 1200, 800, FULLSCREENMODE, &Application::WindProcInit);
		m_Renderer.Create(&m_Window);
		OnInit();
	}
	catch (const std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "Exception", MB_OK);
		return -1;
	}
	m_Window.Show();
	InitializeInput();

	MSG msg = {};

	while (msg.message != WM_QUIT)
	{
		if (GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;

}

void Application::Resize()
{
	m_Window.ResizedWindow();
	m_Camera.SetAspectRatio((float)m_Window.GetClientWidth() / m_Window.GetClientHeight());
	if (m_Renderer.m_SwapChain.Resize())
	{
		CreateRaytracingOutputBuffer();
	}
}

void Application::Update()
{
	ID3D12Device11* device = m_Renderer.GetDevice();
	HeapManager* heapManager = m_Renderer.GetHeap();
	ID3D12CommandAllocator* commandAllocator = m_Renderer.m_CommandAllocator.Get();
	ID3D12GraphicsCommandList6* commandList = m_Renderer.GetCommandList();

	ThrowIfFailed(commandAllocator->Reset());
	ThrowIfFailed(commandList->Reset(commandAllocator, nullptr));

	UpdateFrameTime();

	heapManager->ResetHeapOffset(ScratchDefaultHeap);
	heapManager->ResetHeapOffset(ScratchUploadHeap);

	angle1 += 0.512465799111f * m_FrameTime;
	angle2 += 0.812465799111f * m_FrameTime * 0.38712f;

	BuildScene(commandList);

	SetTitle();
	Input::Update();
	m_Camera.Update(m_FrameTime);
}

void Application::Render()
{
	ID3D12CommandAllocator* commandAllocator = m_Renderer.m_CommandAllocator.Get();
	ID3D12GraphicsCommandList6* commandList = m_Renderer.GetCommandList();

	//ThrowIfFailed(commandAllocator->Reset());
	//ThrowIfFailed(commandList->Reset(commandAllocator, nullptr));

	m_Renderer.m_SwapChain.TransitionBackBuffer(commandList, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);

	TransitionResource(commandList, m_outputResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	//Do Ray Tracing on Output now

	commandList->SetDescriptorHeaps(1, m_Renderer.GetDescriptorHeap()->GetAddressOfHeap());

	//commandList->SetComputeRootSignature(m_GlobalRootSignature.Get());

	D3D12_DISPATCH_RAYS_DESC DispatchDesc = {};

	DispatchDesc.RayGenerationShaderRecord.StartAddress = m_ShaderBindingTable->GetGPUVirtualAddress();
	DispatchDesc.RayGenerationShaderRecord.SizeInBytes = 64;
	DispatchDesc.MissShaderTable.StartAddress = DispatchDesc.RayGenerationShaderRecord.StartAddress + DispatchDesc.RayGenerationShaderRecord.SizeInBytes;
	DispatchDesc.MissShaderTable.SizeInBytes = 64;
	DispatchDesc.MissShaderTable.StrideInBytes = 64;
	DispatchDesc.HitGroupTable.StartAddress = DispatchDesc.MissShaderTable.StartAddress + DispatchDesc.MissShaderTable.SizeInBytes;
	DispatchDesc.HitGroupTable.SizeInBytes = 64;
	DispatchDesc.HitGroupTable.StrideInBytes = 64;
	DispatchDesc.CallableShaderTable;
	DispatchDesc.Width = m_Window.GetClientWidth();
	DispatchDesc.Height = m_Window.GetClientHeight();
	DispatchDesc.Depth = 1;

	commandList->SetPipelineState1(m_rtStateObject.Get());
	// Dispatch the rays and write to the raytracing output
	commandList->DispatchRays(&DispatchDesc);


	//Copy Output to Backbuffer
	TransitionResource(commandList, m_outputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_Renderer.m_SwapChain.CopyResourceToBackBuffer(commandList, m_outputResource.Get());

	m_Renderer.m_SwapChain.TransitionBackBuffer(commandList, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

	ThrowIfFailed(commandList->Close());

	m_Renderer.m_CommandQueue.ExecuteCommandList(commandList);
	m_Renderer.m_SwapChain.Present();
	m_Renderer.m_CommandQueue.Flush();
}

void Application::Exit() const
{
	DestroyWindow(m_Window.GetHandle());
}

void Application::InitializeInput()
{
	RAWINPUTDEVICE Rid = {};
	Rid.usUsagePage = 0x01;
	Rid.usUsage = 0x02;
	Rid.dwFlags = 0;
	Rid.hwndTarget = m_Window.GetHandle();

	RegisterRawInputDevices(&Rid, 1, sizeof(Rid));
}

float Application::GetFPS() const
{
	float fps = 0.0;
	if (m_FrameTime > 0.0001)
	{
		static UINT frameCount = 0;
		const UINT CountMax = 120;
		static float accumulator[CountMax] = {};
		if (frameCount >= CountMax)
		{
			frameCount = 0;
		}
		accumulator[frameCount++] = 1.0f / m_FrameTime;

		for (int i = 0; i < CountMax; i++)
		{
			fps += accumulator[i];
		}
		fps /= CountMax;
	}
	return fps;
};

void Application::UpdateFrameTime()
{
	static std::chrono::high_resolution_clock clock;
	static auto t0 = clock.now();

	auto t1 = clock.now();
	auto deltaTime = t1 - t0;
	t0 = t1;

	m_FrameTime = deltaTime.count() * 1e-9f;
}

void Application::SetTitle() const
{
	swprintf_s(m_TitleBuffer, TITLE_BUFFER_SIZE, L"%s Width:%d Height:%d FPS:%f\n", WINDOWTITLE, m_Window.GetClientWidth(), m_Window.GetClientHeight(), GetFPS());
	SetWindowTextW(m_Window.GetHandle(), m_TitleBuffer);
}

void Application::OnInit()
{
	m_Camera.CreateResource(m_Renderer.GetDevice(), m_Renderer.GetHeap(), &m_Renderer.m_DescriptorHeap);

	m_Scene.Init(m_Renderer.GetDevice(), m_Renderer.GetHeap());
	BuildAssets(m_Renderer.GetCommandList());

	m_rayGenLibrary = CompileShaderLibrary(L"Shaders/RayGen.hlsl");
	m_missLibrary = CompileShaderLibrary(L"Shaders/Miss.hlsl");
	m_hitLibrary = CompileShaderLibrary(L"Shaders/Hit.hlsl");

	CreateRootSignatures(m_Renderer.GetDevice());

	CreateRaytracingPipeline(m_Renderer.GetDevice());


	CreateRaytracingOutputBuffer();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = m_Scene.GetGPUVirtualAddress();
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_Renderer.m_DescriptorHeap.GetCPUHandle(1);
	m_Renderer.GetDevice()->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);

	CreateShaderBindingTable(m_Renderer.GetDevice(), m_Renderer.GetDescriptorHeap());

	m_Renderer.ExecuteCommandList();
}

void Application::BuildAssets(ID3D12GraphicsCommandList6* commandList)
{
	std::vector<Vertex> vertices =
	{ {0.25,0.25,0.25},{0.25,-0.25,0.25},{0.25,0.25,-0.25},{0.25,-0.25,-0.25},{-0.25,0.25,0.25},{-0.25,-0.25,0.25},{-0.25,0.25,-0.25},{-0.25,-0.25,-0.25} };
	std::vector<UINT> indices = { 2,4,0,7,2,3,5,6,7,7,1,5,3,0,1,1,4,5,6,4,2,6,2,7,4,6,5,3,1,7,2,0,3,0,4,1 };
	MeshData cube = { vertices, indices, sizeof(Vertex) };

	std::vector<StructuredVertex> structuredVertex;
	Vertex Color = { 0.5,0.5,0.5 };
	for (int i = 0; i < indices.size(); i += 3)
	{
		Vertex v1 = vertices[indices[i]];
		Vertex v2 = vertices[indices[i + 1]];
		Vertex v3 = vertices[indices[i + 2]];
		Vertex normal = Cross(Subtract(v2, v1), Subtract(v3, v1));
		StructuredVertex sv = {};
		sv.normal = normal;
		sv.color = Color;
		structuredVertex.push_back(sv);
	}

	UINT64 size = structuredVertex.size() * sizeof(StructuredVertex);
	m_StructuredBuffer.CreateResource(m_Renderer.GetDevice(), m_Renderer.GetHeap(), &m_Renderer.m_DescriptorHeap, size, 3);
	m_StructuredBuffer.Upload(structuredVertex.data(), size);
	m_Scene.AddMesh(commandList, MeshCube, &cube);

	BuildScene(commandList);
}

void Application::BuildScene(ID3D12GraphicsCommandList6* commandList)
{
	m_Scene.Reset();
	XMVECTOR quat = XMQuaternionRotationAxis({ 1.5f,4.0f,13.0f }, angle2 * 2.3f);
	XMVECTOR quat2 = XMQuaternionRotationAxis({ 1.5f,1.0f,4.0f }, angle2 * 8.3f);
	XMMATRIX matrix1 = XMMatrixRotationZ(angle2) * XMMatrixTranslation(1, 0, 0) * XMMatrixRotationZ(angle1);
	XMMATRIX matrix2 = XMMatrixTranslation(0.2f, 1.0f, 0.0f) * XMMatrixRotationQuaternion(quat);
	XMMATRIX matrix3 = XMMatrixRotationZ(angle2 * -2.0f);
	XMMATRIX matrix4 = XMMatrixRotationQuaternion(quat2) * XMMatrixTranslation(0.0f, 0.0f, 1.6f);
	XMMATRIX matrixIdentity = XMMatrixIdentity();
	m_Scene.AddInstance(MeshCube, &matrix1, 0, 0);
	m_Scene.AddInstance(MeshCube, &matrix2, 0, 0);
	m_Scene.AddInstance(MeshCube, &matrix3, 0, 0);
	m_Scene.AddInstance(MeshCube, &matrix4, 0, 0);
	m_Scene.Build(commandList);
}

void Application::CreateRaytracingPipeline(ID3D12Device11* device)
{
	const WCHAR* exports[3] = { RayGenName, MissName, ClosestHitName };

	D3D12_EXPORT_DESC exportdesc[3] = {};
	exportdesc[0].Name = exports[0];
	exportdesc[1].Name = exports[1];
	exportdesc[2].Name = exports[2];

	StateObjectGenerator stateObjectGenerator;
	stateObjectGenerator.AddDXIL_Library(m_rayGenLibrary->GetBufferPointer(), m_rayGenLibrary->GetBufferSize(), &exportdesc[0], 1);
	stateObjectGenerator.AddDXIL_Library(m_missLibrary->GetBufferPointer(), m_missLibrary->GetBufferSize(), &exportdesc[1], 1);
	stateObjectGenerator.AddDXIL_Library(m_hitLibrary->GetBufferPointer(), m_hitLibrary->GetBufferSize(), &exportdesc[2], 1);
	stateObjectGenerator.AddHitGroup(HitGroupName, D3D12_HIT_GROUP_TYPE_TRIANGLES, nullptr, ClosestHitName, nullptr);
	int shaderConfigIndex = stateObjectGenerator.AddShaderConfig(5 * sizeof(float), 2 * sizeof(float));
	stateObjectGenerator.AddExportsAssociation(exports, 3, shaderConfigIndex);
	int raygenRootSigIndex = stateObjectGenerator.AddLocalRootSignature(m_rayGenSignature.Get());
	int missRootSigIndex = stateObjectGenerator.AddLocalRootSignature(m_missSignature.Get());
	int hitRootSigIndex = stateObjectGenerator.AddLocalRootSignature(m_hitSignature.Get());
	stateObjectGenerator.AddExportsAssociation(&exports[0], 1, raygenRootSigIndex);
	stateObjectGenerator.AddExportsAssociation(&exports[1], 1, missRootSigIndex);
	stateObjectGenerator.AddExportsAssociation(&exports[2], 1, hitRootSigIndex);
	stateObjectGenerator.AddPipelineConfig(31);
	m_rtStateObject = stateObjectGenerator.Build(device);
}

void Application::CreateRaytracingPipelineOLD(ID3D12Device11* device)
{
	const WCHAR* exports[3] = { RayGenName, MissName, ClosestHitName };

	D3D12_EXPORT_DESC exportdesc[3] = {};
	exportdesc[0].Name = exports[0];
	exportdesc[1].Name = exports[1];
	exportdesc[2].Name = exports[2];

	D3D12_DXIL_LIBRARY_DESC lib_Desc[3] = {};
	lib_Desc[0].DXILLibrary.pShaderBytecode = m_rayGenLibrary->GetBufferPointer();
	lib_Desc[0].DXILLibrary.BytecodeLength = m_rayGenLibrary->GetBufferSize();
	lib_Desc[0].NumExports = 1;
	lib_Desc[0].pExports = &exportdesc[0];

	lib_Desc[1].DXILLibrary.pShaderBytecode = m_missLibrary->GetBufferPointer();
	lib_Desc[1].DXILLibrary.BytecodeLength = m_missLibrary->GetBufferSize();
	lib_Desc[1].NumExports = 1;
	lib_Desc[1].pExports = &exportdesc[1];

	lib_Desc[2].DXILLibrary.pShaderBytecode = m_hitLibrary->GetBufferPointer();
	lib_Desc[2].DXILLibrary.BytecodeLength = m_hitLibrary->GetBufferSize();
	lib_Desc[2].NumExports = 1;
	lib_Desc[2].pExports = &exportdesc[2];

	D3D12_STATE_SUBOBJECT subobjects[13] = {};
	subobjects[0].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobjects[0].pDesc = &lib_Desc[0];

	subobjects[1].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobjects[1].pDesc = &lib_Desc[1];

	subobjects[2].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobjects[2].pDesc = &lib_Desc[2];

	D3D12_HIT_GROUP_DESC hitgroup = {};
	hitgroup.HitGroupExport = HitGroupName;
	hitgroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
	hitgroup.AnyHitShaderImport = nullptr;
	hitgroup.ClosestHitShaderImport = ClosestHitName;
	hitgroup.IntersectionShaderImport = nullptr;

	subobjects[3].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	subobjects[3].pDesc = &hitgroup;

	D3D12_RAYTRACING_SHADER_CONFIG shaderconfig = {};
	shaderconfig.MaxPayloadSizeInBytes = 5 * sizeof(float);
	shaderconfig.MaxAttributeSizeInBytes = 2 * sizeof(float);

	subobjects[4].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	subobjects[4].pDesc = &shaderconfig;

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderPayloadAssociation = {};
	shaderPayloadAssociation.pSubobjectToAssociate = &subobjects[4];
	shaderPayloadAssociation.NumExports = 3;
	shaderPayloadAssociation.pExports = exports;

	subobjects[5].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	subobjects[5].pDesc = &shaderPayloadAssociation;

	//------------------------------
	D3D12_LOCAL_ROOT_SIGNATURE localrootSignatures[3] = {};
	localrootSignatures[0].pLocalRootSignature = m_rayGenSignature.Get();
	localrootSignatures[1].pLocalRootSignature = m_missSignature.Get();
	localrootSignatures[2].pLocalRootSignature = m_hitSignature.Get();

	subobjects[6].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	subobjects[6].pDesc = &localrootSignatures[0];

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION exportsAssociation[3] = {};
	exportsAssociation[0].pSubobjectToAssociate = &subobjects[6];
	exportsAssociation[0].NumExports = 1;
	exportsAssociation[0].pExports = &exports[0];

	subobjects[7].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	subobjects[7].pDesc = &exportsAssociation[0];


	subobjects[8].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	subobjects[8].pDesc = &localrootSignatures[1];

	exportsAssociation[1].pSubobjectToAssociate = &subobjects[8];
	exportsAssociation[1].NumExports = 1;
	exportsAssociation[1].pExports = &exports[1];

	subobjects[9].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	subobjects[9].pDesc = &exportsAssociation[1];

	subobjects[10].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	subobjects[10].pDesc = &localrootSignatures[2];

	exportsAssociation[2].pSubobjectToAssociate = &subobjects[10];
	exportsAssociation[2].NumExports = 1;
	exportsAssociation[2].pExports = &exports[2];

	subobjects[11].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	subobjects[11].pDesc = &exportsAssociation[2];

	//------------------------------
	//D3D12_GLOBAL_ROOT_SIGNATURE globalRootSig = {};
	//globalRootSig.pGlobalRootSignature = m_GlobalRootSignature.Get();

	//subobjects[12].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	//subobjects[12].pDesc = &globalRootSig;


	//D3D12_LOCAL_ROOT_SIGNATURE localRootSig = {};
	//localRootSig.pLocalRootSignature = m_LocalRootSignature.Get();

	//subobjects[13].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	//subobjects[13].pDesc = &localRootSig;


	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
	pipelineConfig.MaxTraceRecursionDepth = 31; // Test this

	subobjects[12].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	subobjects[12].pDesc = &pipelineConfig;


	// Describe the ray tracing pipeline state object
	D3D12_STATE_OBJECT_DESC pipelineDesc = {};
	pipelineDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	pipelineDesc.NumSubobjects = 13;
	pipelineDesc.pSubobjects = subobjects;

	ThrowIfFailed(device->CreateStateObject(&pipelineDesc, IID_PPV_ARGS(&m_rtStateObject)));

	//UINT64 subobjectCount =
	//	m_libraries.size() +                     // DXIL libraries
	//	m_hitGroups.size() +                     // Hit group declarations
	//	1 +                                      // Shader configuration
	//	1 +                                      // Shader payload
	//	2 * m_rootSignatureAssociations.size() + // Root signature declaration + association
	//	2 +                                      // Empty global and local root signatures
	//	1;                                       // Final pipeline subobject

}

void Application::CreateRootSignatures(ID3D12Device11* device)
{
	//RootSignatureGenerator globalRootSignatureGenerator(device, D3D12_ROOT_SIGNATURE_FLAG_NONE);
	//m_GlobalRootSignature = globalRootSignatureGenerator.Generate();

	//RootSignatureGenerator localRootSignatureGenerator(device, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
	//m_LocalRootSignature = localRootSignatureGenerator.Generate();
	{
		D3D12_DESCRIPTOR_RANGE ranges[2] = {};
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		ranges[0].NumDescriptors = 1;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace = 0;
		ranges[0].OffsetInDescriptorsFromTableStart = 0;

		ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[1].NumDescriptors = 2;
		ranges[1].BaseShaderRegister = 0;
		ranges[1].RegisterSpace = 0;
		ranges[1].OffsetInDescriptorsFromTableStart = 1;

		D3D12_ROOT_PARAMETER parameter = {};
		parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		parameter.DescriptorTable.NumDescriptorRanges = 2;
		parameter.DescriptorTable.pDescriptorRanges = ranges;
		parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		RootSignatureGenerator RayGenRootSignatureGenerator(device, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
		RayGenRootSignatureGenerator.AddParameter(parameter);
		m_rayGenSignature = RayGenRootSignatureGenerator.Generate();
	}
	{
		RootSignatureGenerator missRootSignatureGenerator(device, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
		m_missSignature = missRootSignatureGenerator.Generate();
	}
	{
		D3D12_DESCRIPTOR_RANGE ranges[1] = {};
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[0].NumDescriptors = 3;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace = 0;
		ranges[0].OffsetInDescriptorsFromTableStart = 1;

		D3D12_ROOT_PARAMETER parameter = {};
		parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		parameter.DescriptorTable.NumDescriptorRanges = 1;
		parameter.DescriptorTable.pDescriptorRanges = ranges;
		parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		RootSignatureGenerator hitRootSignatureGenerator(device, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
		hitRootSignatureGenerator.AddParameter(parameter);
		m_hitSignature = hitRootSignatureGenerator.Generate();
	}
}

void Application::CreateRaytracingOutputBuffer()
{
	ID3D12Device11* device = m_Renderer.GetDevice();

	D3D12_HEAP_PROPERTIES heapProp = { };
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	// The backbuffer is actually DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, but sRGB 
	// formats cannot be used with UAVs. For accuracy we should convert to sRGB 
	// ourselves in the shader 
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resDesc.Width = m_Window.GetClientWidth();
	resDesc.Height = m_Window.GetClientHeight();
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;

	device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&m_outputResource));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_Renderer.m_DescriptorHeap.GetCPUHandle(0);
	device->CreateUnorderedAccessView(m_outputResource.Get(), nullptr, &uavDesc, srvHandle);
}

void Application::CreateShaderBindingTable(ID3D12Device11* device, DescriptorHeap* descriptorHeap)
{
	D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle = descriptorHeap->GetGPUHandle(0);
	ComPtr<ID3D12StateObjectProperties> stateObjectProps;
	UINT shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	ThrowIfFailed(m_rtStateObject.As(&stateObjectProps));

	void* rayGenShaderIdentifier = stateObjectProps->GetShaderIdentifier(RayGenName);
	void* missShaderIdentifier = stateObjectProps->GetShaderIdentifier(MissName);
	void* hitGroupShaderIdentifier = stateObjectProps->GetShaderIdentifier(HitGroupName);

	UINT64 elementSize = 64ULL * 3;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Alignment = 0;
	resDesc.Width = elementSize;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	m_ShaderBindingTable = m_Renderer.GetHeap()->CreateResource(device, UploadHeap, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ);

	void* Source = nullptr;
	{
		void* DestinationStart = nullptr;
		const D3D12_RANGE readRange = { 0, 0 };
		ThrowIfFailed(m_ShaderBindingTable->Map(0, &readRange, &DestinationStart));
		void* Destination = DestinationStart;

		Source = rayGenShaderIdentifier;
		memcpy(Destination, Source, shaderIdentifierSize);
		Destination = (UINT8*)Destination + shaderIdentifierSize;

		//Copy Arguments
		{
			D3D12_GPU_DESCRIPTOR_HANDLE descriptorhandle = m_Renderer.m_DescriptorHeap.GetGPUHandle(0);
			UINT64 data = (UINT64)descriptorhandle.ptr;
			memcpy(Destination, &data, sizeof(data));
			Destination = (UINT8*)Destination + sizeof(data);
		}

		Source = missShaderIdentifier;
		Destination = (UINT8*)DestinationStart + 64ULL;
		memcpy(Destination, Source, shaderIdentifierSize);

		Source = hitGroupShaderIdentifier;
		Destination = (UINT8*)DestinationStart + 64ULL * 2;
		memcpy(Destination, Source, shaderIdentifierSize);
		Destination = (UINT8*)Destination + shaderIdentifierSize;

		//Copy Arguments
		{
			D3D12_GPU_DESCRIPTOR_HANDLE descriptorhandle = m_Renderer.m_DescriptorHeap.GetGPUHandle(0);
			UINT64 data = (UINT64)descriptorhandle.ptr;
			memcpy(Destination, &data, sizeof(data));
			Destination = (UINT8*)Destination + sizeof(data);
		}

		m_ShaderBindingTable->Unmap(0, nullptr);
	}

	TransitionResource(m_Renderer.GetCommandList(), m_ShaderBindingTable.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

LRESULT CALLBACK Application::WindProcInit(HWND hwindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (AppPtr)
		return AppPtr->WindProc(hwindow, message, wParam, lParam);
	else
	{
		if (message == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			AppPtr = reinterpret_cast<Application*>(pCreate->lpCreateParams);
			return AppPtr->WindProc(hwindow, message, wParam, lParam);
		}
		return DefWindowProc(hwindow, message, wParam, lParam);
	}
}

LRESULT CALLBACK Application::WindProc(HWND hwindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	{
		switch (message)
		{
		case WM_CREATE:
		{
			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
		case WM_ERASEBKGND:
		{
			return 1;
		}
		case WM_PAINT:
		{
			Update();
			Render();
			return 0;
		}
		case WM_KILLFOCUS:
		{
			Input::KillFocus();
			break;
		}
		case WM_SIZE:
		{
			Resize();
			return 0;
		}
		break;
		case WM_CLOSE:
		{
			Exit();
			break;
		}
		case WM_INPUT:
		{
			UINT dataSize = 0;
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dataSize, sizeof(RAWINPUTHEADER));
			RAWINPUT pData = {};
			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, (void*)&pData, &dataSize, sizeof(RAWINPUTHEADER)) == dataSize)
			{
				Input::RawMouseAccumulator(pData.data.mouse.lLastX, pData.data.mouse.lLastY);
			}
			break;
		}
		case WM_MOUSEMOVE:
		{
			Input::UpdateMouse(lParam, &m_Window);
			break;
		}
		case WM_LBUTTONDOWN:
		{
			SetCapture(hwindow);
			Input::HideMouseCursor(true);
			Input::SetButtonPressedState(0, true);
			break;
		}
		case WM_LBUTTONUP:
		{
			ReleaseCapture();
			Input::HideMouseCursor(false);
			Input::SetButtonPressedState(0, false);
			break;
		}
		case WM_RBUTTONDOWN:
		{
			Input::SetButtonPressedState(1, true);
			break;
		}
		case WM_RBUTTONUP:
		{
			Input::SetButtonPressedState(1, false);
			break;
		}
		case WM_MBUTTONDOWN:
		{
			Input::SetButtonPressedState(2, true);
			break;
		}
		case WM_MBUTTONUP:
		{
			Input::SetButtonPressedState(2, false);
			break;
		}
		case WM_XBUTTONDOWN:
		{
			if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
			{
				Input::SetButtonPressedState(3, true);
			}
			else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2)
			{
				Input::SetButtonPressedState(4, true);
			}
			break;
		}
		case WM_XBUTTONUP:
		{
			if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
			{
				Input::SetButtonPressedState(3, false);
			}
			else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2)
			{
				Input::SetButtonPressedState(4, false);
			}
			break;
		}
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			Input::SetKeyPressedState((UINT)wParam, true);
			bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
			switch (wParam)
			{
			case 'V':
				m_Renderer.ToggleVSync();
				break;
			case VK_ESCAPE:
				Exit();
				break;
			case VK_RETURN:
				if (alt)
				{
			case VK_F11:
				m_Window.ToggleFullScreen();
				return 0;
				}
			case VK_F4:
				if (alt)
				{
					Exit();
				}
			default:
				break;
			}
		}
		case WM_SYSCHAR:
		{
			return 0;
		}
		case WM_KEYUP:
		{
			Input::SetKeyPressedState((UINT)wParam, false);
			break;
		}
		default:
			break;
		}
	}
	return DefWindowProc(hwindow, message, wParam, lParam);
}
