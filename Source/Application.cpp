#include "PCH.h"
#include "Application.h"
#include "Input.h"
#include "DX12Utility.h"
#include "RaytracingPipelineGenerator.h"
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
	m_Window.Create(m_hInstance, this, WINDOWTITLE, 1200, 800, FULLSCREENMODE, &Application::WindProcInit);
	if (!m_Window.GetHandle())
		return 1;
	m_Renderer.Create(&m_Window);
	BuildAssets(m_Renderer.GetDevice(), m_Renderer.GetCommandList());
	CreateRaytracingOutputBuffer(m_Renderer.GetDevice(), m_Renderer.GetHeap());
	CreateShaderResourceHeap(m_Renderer.GetDevice(), m_Renderer.GetDescriptorHeap());
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

void Application::Update()
{
	UpdateFrameTime();
	SetTitle();
	Input::Update();
}

void Application::Render()
{
	m_Renderer.StartNextFrame();
	m_Renderer.Present();
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
	swprintf_s(m_TitleBuffer, TITLE_BUFFER_SIZE, L"%s Width:%d Height:%d FPS:%f Size: %zd\n", WINDOWTITLE, m_Window.GetClientWidth(), m_Window.GetClientHeight(), GetFPS(), sizeof(Application));
	SetWindowTextW(m_Window.GetHandle(), m_TitleBuffer);
}

void Application::BuildAssets(ID3D12Device11* device, ID3D12GraphicsCommandList6* commandList)
{
	// Data
	Vertex vertices[] =
	{
		{ -0.5f, -0.5f, 0.0f },
		{ 0.5f, -0.5f, 0.0f },
		{ 0.0f, 0.5f, 0.0f }
	};

	UINT indices[] = { 0,1,2 };

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
		geometry_Desc.Triangles.Transform3x4 = Matrix->GetGPUVirtualAddress(); // GPU Resource Address
		geometry_Desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
		geometry_Desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geometry_Desc.Triangles.IndexCount = 3;
		geometry_Desc.Triangles.VertexCount = 3;
		geometry_Desc.Triangles.IndexBuffer = IndexBuffer->GetGPUVirtualAddress(); // GPU Resource Address
		geometry_Desc.Triangles.VertexBuffer.StartAddress = VertexBuffer->GetGPUVirtualAddress(); // GPU Resource Address
		geometry_Desc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex); // GPU Resource Address

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

		//Instance Desc
		D3D12_RAYTRACING_INSTANCE_DESC instanceDescs = {};

		// Instance transform matrix
		DirectX::XMMATRIX m = XMMatrixTranspose(matrix); // GLM is column major, the INSTANCE_DESC is row major
		memcpy(&instanceDescs.Transform, &m, sizeof(instanceDescs.Transform));

		// Instance ID visible in the shader in InstanceID()
		instanceDescs.InstanceID = 0;

		// Visibility mask, always visible here
		instanceDescs.InstanceMask = 0xFF;

		// Index of the hit group invoked upon intersection
		instanceDescs.InstanceContributionToHitGroupIndex = 0;

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

	CreateRaytracingPipeline(device);




	m_Renderer.ExecuteCommandList();
}

//-----------------------------------------------------------------------------
// The ray generation shader needs to access 2 resources: the raytracing output
// and the top-level acceleration structure
//
ComPtr<ID3D12RootSignature> Application::CreateRayGenSignature(ID3D12Device11* device)
{
	nv_helpers_dx12::RootSignatureGenerator rsc;
	rsc.AddHeapRangesParameter(
		{
			{0 /*u0*/, 1 /*1 descriptor */, 0 /*use the implicit register space 0*/, D3D12_DESCRIPTOR_RANGE_TYPE_UAV /* UAV representing the output buffer*/, 0 /*heap slot where the UAV is defined*/},
			{0 /*t0*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV /*Top-level acceleration structure*/, 1}
		}
	);
	return rsc.Generate(device, true);
}

//-----------------------------------------------------------------------------
// The hit shader communicates only through the ray payload, and therefore does
// not require any resources
//
ComPtr<ID3D12RootSignature> Application::CreateHitSignature(ID3D12Device11* device)
{
	nv_helpers_dx12::RootSignatureGenerator rsc;
	return rsc.Generate(device, true);
}

//-----------------------------------------------------------------------------
// The miss shader communicates only through the ray payload, and therefore
// does not require any resources
//
ComPtr<ID3D12RootSignature> Application::CreateMissSignature(ID3D12Device11* device)
{
	nv_helpers_dx12::RootSignatureGenerator rsc;
	return rsc.Generate(device, true);
}

//-----------------------------------------------------------------------------
//
// The raytracing pipeline binds the shader code, root signatures and pipeline
// characteristics in a single structure used by DXR to invoke the shaders and
// manage temporary memory during raytracing
//
//
void Application::CreateRaytracingPipeline(ID3D12Device11* device)
{
	nv_helpers_dx12::RayTracingPipelineGenerator pipeline(device);
	// The pipeline contains the DXIL code of all the shaders potentially executed 
	// during the raytracing process. This section compiles the HLSL code into a 
	// set of DXIL libraries. We chose to separate the code in several libraries 
	// by semantic (ray generation, hit, miss) for clarity. Any code layout can be 
	// used. 
	m_rayGenLibrary = CompileShaderLibrary(L"Shaders/RayGen.hlsl");
	m_missLibrary = CompileShaderLibrary(L"Shaders/Miss.hlsl");
	m_hitLibrary = CompileShaderLibrary(L"Shaders/Hit.hlsl");


	// To be used, each DX12 shader needs a root signature defining which 
	// parameters and buffers will be accessed. 
	m_rayGenSignature = CreateRayGenSignature(device);
	m_missSignature = CreateHitSignature(device);
	m_hitSignature = CreateMissSignature(device);

	CreateDummyRootSignatures(device);

	// To be used, each shader needs to be associated to its root signature.
	// A shaders imported from the DXIL libraries needs to be associated with 
	// exactly one root signature.The shaders comprising the hit groups need 
	// to share the same root signature, which is associated to the 
	// hit group(and not to the shaders themselves).Note that a shader 
	// does not have to actually access all the resources declared in its 
	// root signature, as long as the root signature defines a superset 
	// of the resources the shader needs.

	// The following section associates the root signature to each shader. Note 
	// that we can explicitly show that some shaders share the same root signature 
	// (eg. Miss and ShadowMiss). Note that the hit shaders are now only referred 
	// to as hit groups, meaning that the underlying intersection, any-hit and 
	// closest-hit shaders share the same root signature. 

	const WCHAR* RayGenName = L"RayGen";
	const WCHAR* MissName = L"Miss";
	const WCHAR* ClosestHitName = L"ClosestHit";
	const WCHAR* HitGroupName = L"HitGroup";

	const WCHAR* exports[3] = { RayGenName, MissName, ClosestHitName };

	D3D12_EXPORT_DESC exportdesc[3] = {};
	exportdesc[0].Name = RayGenName;
	exportdesc[1].Name = MissName;
	exportdesc[2].Name = ClosestHitName;

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


	D3D12_GLOBAL_ROOT_SIGNATURE globalRootSig = {};
	globalRootSig.pGlobalRootSignature = m_DummyGlobalRootSignature.Get();

	subobjects[12].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	subobjects[12].pDesc = &globalRootSig;


	D3D12_LOCAL_ROOT_SIGNATURE localRootSig = {};
	localRootSig.pLocalRootSignature = m_DummyLocalRootSignature.Get();

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

void Application::CreateDummyRootSignatures(ID3D12Device11* device)
{
	// Creation of the global root signature
	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
	rootDesc.NumParameters = 0;
	rootDesc.pParameters = nullptr;
	// A global root signature is the default, hence this flag
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	HRESULT hr = 0;

	ID3DBlob* serializedRootSignature;
	ID3DBlob* error;

	// Create the empty global root signature
	hr = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		&serializedRootSignature, &error);
	if (FAILED(hr))
	{
		throw std::logic_error("Could not serialize the global root signature");
	}
	hr = device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(),
		serializedRootSignature->GetBufferSize(),
		IID_PPV_ARGS(&m_DummyGlobalRootSignature));

	serializedRootSignature->Release();
	if (FAILED(hr))
	{
		throw std::logic_error("Could not create the global root signature");
	}

	// Create the local root signature, reusing the same descriptor but altering the creation flag
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	hr = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		&serializedRootSignature, &error);
	if (FAILED(hr))
	{
		throw std::logic_error("Could not serialize the local root signature");
	}
	hr = device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(),
		serializedRootSignature->GetBufferSize(),
		IID_PPV_ARGS(&m_DummyLocalRootSignature));

	serializedRootSignature->Release();
	if (FAILED(hr))
	{
		throw std::logic_error("Could not create the local root signature");
	}
}

//-----------------------------------------------------------------------------
//
// Allocate the buffer holding the raytracing output, with the same size as the
// output image
//
void Application::CreateRaytracingOutputBuffer(ID3D12Device11* device, HeapManager* heap)
{
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
	resDesc.MipLevels = 1; resDesc.SampleDesc.Count = 1;
	m_outputResource = heap->CreateResource(device,DefaultHeap,&resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE);
}

//-----------------------------------------------------------------------------
//
// Create the main heap used by the shaders, which will give access to the
// raytracing output and the top-level acceleration structure
//
void Application::CreateShaderResourceHeap(ID3D12Device11* device, DescriptorHeap* descriptorHeap)
{ 
	descriptorHeap->Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = descriptorHeap->GetCPUHandle(0);
	device->CreateUnorderedAccessView(m_outputResource.Get(), nullptr, &uavDesc, srvHandle);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = TLASResult->GetGPUVirtualAddress();
	srvHandle = descriptorHeap->GetCPUHandle(1);
	device->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);
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
			m_Window.ResizedWindow();
			m_Renderer.Resize();
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
