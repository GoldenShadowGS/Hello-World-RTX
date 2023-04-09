#include "PCH.h"
#include "Application.h"
#include "Input.h"
#include "DX12Utility.h"
#include "RootSignatureGenerator.h"

#define WINDOWTITLE L"Hello World RTX"
#define FULLSCREENMODE false
#define TITLE_BUFFER_SIZE 256

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
	UpdateFrameTime();
	SetTitle();
	Input::Update();
	m_Camera.Update(m_FrameTime);
}

void Application::Render()
{
	ID3D12CommandAllocator* commandAllocator = m_Renderer.m_CommandAllocator.Get();
	ID3D12GraphicsCommandList6* commandList = m_Renderer.GetCommandList();

	ThrowIfFailed(commandAllocator->Reset());
	ThrowIfFailed(commandList->Reset(commandAllocator, nullptr));

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
	m_Camera.CreateResource(m_Renderer.GetDevice(), &m_Renderer.m_DescriptorHeap);
	BuildAssets(m_Renderer.GetDevice(), m_Renderer.GetCommandList());

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
	srvDesc.RaytracingAccelerationStructure.Location = TLASResult->GetGPUVirtualAddress();
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_Renderer.m_DescriptorHeap.GetCPUHandle(1);
	m_Renderer.GetDevice()->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);

	CreateShaderBindingTable(m_Renderer.GetDevice(), m_Renderer.GetDescriptorHeap());

	m_Renderer.ExecuteCommandList();
}

void Application::BuildAssets(ID3D12Device11* device, ID3D12GraphicsCommandList6* commandList)
{
	Vertex vertices[] =
	{ {0.25,0.25,0.25},{0.25,-0.25,0.25},{0.25,0.25,-0.25},{0.25,-0.25,-0.25},{-0.25,0.25,0.25},{-0.25,-0.25,0.25},{-0.25,0.25,-0.25},{-0.25,-0.25,-0.25} };

	UINT indices[] = { 2,4,0,7,2,3,5,6,7,7,1,5,3,0,1,1,4,5,6,4,2,6,2,7,4,6,5,3,1,7,2,0,3,0,4,1 };

	using namespace DirectX;
	XMMATRIX matrix = XMMatrixIdentity();

	// UploadHeap Resources
	// Vertex Buffer Resource
	D3D12_RESOURCE_DESC VertexResourceDesc = {};
	VertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	VertexResourceDesc.Alignment = 0;
	VertexResourceDesc.Width = sizeof(vertices);
	VertexResourceDesc.Height = 1;
	VertexResourceDesc.DepthOrArraySize = 1;
	VertexResourceDesc.MipLevels = 1;
	VertexResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	VertexResourceDesc.SampleDesc.Count = 1;
	VertexResourceDesc.SampleDesc.Quality = 0;
	VertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	VertexResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	VertexBuffer = m_Renderer.GetHeap()->CreateResource(device, UploadHeap, &VertexResourceDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	{
		void* Destination = nullptr;
		const void* Source = vertices;
		const UINT Size = sizeof(vertices);
		const D3D12_RANGE readRange = { 0, 0 };
		ThrowIfFailed(VertexBuffer->Map(0, &readRange, &Destination));
		memcpy(Destination, Source, Size);
		VertexBuffer->Unmap(0, nullptr);
	}

	// Index Buffer Resource
	D3D12_RESOURCE_DESC IndexResourceDesc = {};
	IndexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	IndexResourceDesc.Alignment = 0;
	IndexResourceDesc.Width = sizeof(indices);
	IndexResourceDesc.Height = 1;
	IndexResourceDesc.DepthOrArraySize = 1;
	IndexResourceDesc.MipLevels = 1;
	IndexResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	IndexResourceDesc.SampleDesc.Count = 1;
	IndexResourceDesc.SampleDesc.Quality = 0;
	IndexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	IndexResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	IndexBuffer = m_Renderer.GetHeap()->CreateResource(device, UploadHeap, &IndexResourceDesc, D3D12_RESOURCE_STATE_INDEX_BUFFER);
	{
		void* Destination = nullptr;
		const void* Source = indices;
		const UINT Size = sizeof(indices);
		const D3D12_RANGE readRange = { 0, 0 };
		ThrowIfFailed(IndexBuffer->Map(0, &readRange, &Destination));
		memcpy(Destination, Source, Size);
		IndexBuffer->Unmap(0, nullptr);
	}

	// Matrix Resource
	D3D12_RESOURCE_DESC MatrixResourceDesc = {};
	MatrixResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	MatrixResourceDesc.Alignment = 0;
	MatrixResourceDesc.Width = sizeof(XMMATRIX);
	MatrixResourceDesc.Height = 1;
	MatrixResourceDesc.DepthOrArraySize = 1;
	MatrixResourceDesc.MipLevels = 1;
	MatrixResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	MatrixResourceDesc.SampleDesc.Count = 1;
	MatrixResourceDesc.SampleDesc.Quality = 0;
	MatrixResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	MatrixResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	Matrix = m_Renderer.GetHeap()->CreateResource(device, UploadHeap, &MatrixResourceDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	{
		void* Destination = nullptr;
		const void* Source = &matrix;
		const UINT Size = sizeof(XMMATRIX);
		const D3D12_RANGE readRange = { 0, 0 };
		ThrowIfFailed(Matrix->Map(0, &readRange, &Destination));
		memcpy(Destination, Source, Size);
		Matrix->Unmap(0, nullptr);
	}

	// Bottom Level Acceleration Structure
	{
		D3D12_RAYTRACING_GEOMETRY_DESC geometry_Desc = {};
		geometry_Desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometry_Desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		geometry_Desc.Triangles.Transform3x4 = Matrix->GetGPUVirtualAddress();
		geometry_Desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
		geometry_Desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geometry_Desc.Triangles.IndexCount = _countof(indices);
		geometry_Desc.Triangles.VertexCount = _countof(vertices);
		geometry_Desc.Triangles.IndexBuffer = IndexBuffer->GetGPUVirtualAddress();
		geometry_Desc.Triangles.VertexBuffer.StartAddress = VertexBuffer->GetGPUVirtualAddress();
		geometry_Desc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS AS_Inputs = {};
		AS_Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		AS_Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		AS_Inputs.NumDescs = 1;
		AS_Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		AS_Inputs.pGeometryDescs = &geometry_Desc;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info = {};

		device->GetRaytracingAccelerationStructurePrebuildInfo(&AS_Inputs, &prebuild_info);

		UINT64 scratchSize = Align64(prebuild_info.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		UINT64 resultSize = Align64(prebuild_info.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		D3D12_RESOURCE_DESC ScratchResourceDesc = {};
		ScratchResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		ScratchResourceDesc.Alignment = 0;
		ScratchResourceDesc.Width = scratchSize;
		ScratchResourceDesc.Height = 1;
		ScratchResourceDesc.DepthOrArraySize = 1;
		ScratchResourceDesc.MipLevels = 1;
		ScratchResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		ScratchResourceDesc.SampleDesc.Count = 1;
		ScratchResourceDesc.SampleDesc.Quality = 0;
		ScratchResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ScratchResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		BLScratch = m_Renderer.GetHeap()->CreateResource(device, DefaultHeap, &ScratchResourceDesc, D3D12_RESOURCE_STATE_COMMON);

		D3D12_RESOURCE_DESC ResultResourceDesc = {};
		ResultResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		ResultResourceDesc.Alignment = 0;
		ResultResourceDesc.Width = resultSize;
		ResultResourceDesc.Height = 1;
		ResultResourceDesc.DepthOrArraySize = 1;
		ResultResourceDesc.MipLevels = 1;
		ResultResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		ResultResourceDesc.SampleDesc.Count = 1;
		ResultResourceDesc.SampleDesc.Quality = 0;
		ResultResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ResultResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		BLASResult = m_Renderer.GetHeap()->CreateResource(device, DefaultHeap, &ResultResourceDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AS_BuildDesc = {};
		AS_BuildDesc.DestAccelerationStructureData = BLASResult->GetGPUVirtualAddress(); // GPU Resource Address
		AS_BuildDesc.Inputs = AS_Inputs;
		AS_BuildDesc.SourceAccelerationStructureData = 0; // GPU Resource Address
		AS_BuildDesc.ScratchAccelerationStructureData = BLScratch->GetGPUVirtualAddress(); // GPU Resource Address

		commandList->BuildRaytracingAccelerationStructure(&AS_BuildDesc, 0, nullptr);

		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = BLASResult.Get();
		uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		commandList->ResourceBarrier(1, &uavBarrier);
	}
	// Top Level Acceleration Structure
	{
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuildDesc = {};
		prebuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		prebuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		prebuildDesc.NumDescs = 1;
		prebuildDesc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info = {};

		device->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildDesc, &prebuild_info);

		UINT64 scratchSize = Align64(prebuild_info.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		UINT64 resultSize = Align64(prebuild_info.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		UINT64 instanceDescsSize = Align64(sizeof(D3D12_RAYTRACING_INSTANCE_DESC), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		D3D12_RESOURCE_DESC ScratchResourceDesc = {};
		ScratchResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		ScratchResourceDesc.Alignment = 0;
		ScratchResourceDesc.Width = scratchSize;
		ScratchResourceDesc.Height = 1;
		ScratchResourceDesc.DepthOrArraySize = 1;
		ScratchResourceDesc.MipLevels = 1;
		ScratchResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		ScratchResourceDesc.SampleDesc.Count = 1;
		ScratchResourceDesc.SampleDesc.Quality = 0;
		ScratchResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ScratchResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		TLScratch = m_Renderer.GetHeap()->CreateResource(device, DefaultHeap, &ScratchResourceDesc, D3D12_RESOURCE_STATE_COMMON);

		D3D12_RESOURCE_DESC ResultResourceDesc = {};
		ResultResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		ResultResourceDesc.Alignment = 0;
		ResultResourceDesc.Width = resultSize;
		ResultResourceDesc.Height = 1;
		ResultResourceDesc.DepthOrArraySize = 1;
		ResultResourceDesc.MipLevels = 1;
		ResultResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		ResultResourceDesc.SampleDesc.Count = 1;
		ResultResourceDesc.SampleDesc.Quality = 0;
		ResultResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ResultResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		TLASResult = m_Renderer.GetHeap()->CreateResource(device, DefaultHeap, &ResultResourceDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

		D3D12_RESOURCE_DESC DescriptorResourceDesc = {};
		DescriptorResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		DescriptorResourceDesc.Alignment = 0;
		DescriptorResourceDesc.Width = instanceDescsSize;
		DescriptorResourceDesc.Height = 1;
		DescriptorResourceDesc.DepthOrArraySize = 1;
		DescriptorResourceDesc.MipLevels = 1;
		DescriptorResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		DescriptorResourceDesc.SampleDesc.Count = 1;
		DescriptorResourceDesc.SampleDesc.Quality = 0;
		DescriptorResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		DescriptorResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		descriptorsBuffer = m_Renderer.GetHeap()->CreateResource(device, UploadHeap, &DescriptorResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ);

		// TODO experiment with matrix transforms here and in the Bottomlevel
		// Instance transform matrix
		DirectX::XMMATRIX m = XMMatrixTranspose(matrix); // GLM is column major, the INSTANCE_DESC is row major

		//Instance Desc
		D3D12_RAYTRACING_INSTANCE_DESC instanceDescs = {};
		memcpy(&instanceDescs.Transform, &m, sizeof(instanceDescs.Transform));
		instanceDescs.InstanceID = 0; // Instance ID visible in the shader in InstanceID()
		instanceDescs.InstanceMask = 0xFF; // Visibility mask, always visible here
		instanceDescs.InstanceContributionToHitGroupIndex = 0; // Index of the hit group invoked upon intersection
		instanceDescs.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		instanceDescs.AccelerationStructure = BLASResult->GetGPUVirtualAddress();

		//Copy the instance Desc into the resource
		{
			void* Destination = nullptr;
			const void* Source = &instanceDescs;
			const UINT Size = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
			const D3D12_RANGE readRange = { 0, 0 };
			ThrowIfFailed(descriptorsBuffer->Map(0, &readRange, &Destination));
			memcpy(Destination, Source, Size);
			descriptorsBuffer->Unmap(0, nullptr);
		}

		// Create a descriptor of the requested builder work, to generate a top-level AS from the input parameters
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
		buildDesc.DestAccelerationStructureData = { TLASResult->GetGPUVirtualAddress() };
		buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		buildDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		buildDesc.Inputs.NumDescs = 1;
		buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		buildDesc.Inputs.InstanceDescs = descriptorsBuffer->GetGPUVirtualAddress();
		buildDesc.ScratchAccelerationStructureData = TLScratch->GetGPUVirtualAddress();
		buildDesc.SourceAccelerationStructureData = 0;

		// Build the top-level AS
		commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

		// Wait for the builder to complete by setting a barrier on the resulting
		// buffer. This can be important in case the rendering is triggered
		// immediately afterwards, without executing the command list
		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = TLASResult.Get();
		uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		commandList->ResourceBarrier(1, &uavBarrier);

	}
}

void Application::CreateRaytracingPipeline(ID3D12Device11* device)
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

	D3D12_STATE_SUBOBJECT subobjects[15] = {};
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
	shaderconfig.MaxPayloadSizeInBytes = 4 * sizeof(float);
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
	localrootSignatures[1].pLocalRootSignature = m_hitSignature.Get();
	localrootSignatures[2].pLocalRootSignature = m_missSignature.Get();

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
	D3D12_GLOBAL_ROOT_SIGNATURE globalRootSig = {};
	globalRootSig.pGlobalRootSignature = m_GlobalRootSignature.Get();

	subobjects[12].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	subobjects[12].pDesc = &globalRootSig;


	D3D12_LOCAL_ROOT_SIGNATURE localRootSig = {};
	localRootSig.pLocalRootSignature = m_LocalRootSignature.Get();

	subobjects[13].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	subobjects[13].pDesc = &localRootSig;


	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
	pipelineConfig.MaxTraceRecursionDepth = 1;

	subobjects[14].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	subobjects[14].pDesc = &pipelineConfig;


	// Describe the ray tracing pipeline state object
	D3D12_STATE_OBJECT_DESC pipelineDesc = {};
	pipelineDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	pipelineDesc.NumSubobjects = 15;
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
	RootSignatureGenerator globalRootSignatureGenerator(device, D3D12_ROOT_SIGNATURE_FLAG_NONE);
	m_GlobalRootSignature = globalRootSignatureGenerator.Generate();

	RootSignatureGenerator localRootSignatureGenerator(device, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
	m_LocalRootSignature = localRootSignatureGenerator.Generate();

	RootSignatureGenerator RayGenRootSignatureGenerator(device, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

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

	RayGenRootSignatureGenerator.AddParameter(parameter);
	m_rayGenSignature = RayGenRootSignatureGenerator.Generate();

	RootSignatureGenerator hitRootSignatureGenerator(device, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
	m_hitSignature = hitRootSignatureGenerator.Generate();

	RootSignatureGenerator missRootSignatureGenerator(device, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
	m_missSignature = missRootSignatureGenerator.Generate();
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

	UINT shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	ThrowIfFailed(m_rtStateObject.As(&m_rtStateObjectProps));

	void* rayGenShaderIdentifier = m_rtStateObjectProps->GetShaderIdentifier(RayGenName);
	void* missShaderIdentifier = m_rtStateObjectProps->GetShaderIdentifier(MissName);
	void* hitGroupShaderIdentifier = m_rtStateObjectProps->GetShaderIdentifier(HitGroupName);

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
