#include "PCH.h"
#include "Camera.h"
#include "Input.h"
#include "Renderer.h"
#include "SwapChain.h"
#include "DescriptorHeap.h"
#include "MathUtility.h"
#include "DX12Utility.h"

#define MinPitch -DirectX::XM_PI / 2.0
#define MaxPitch DirectX::XM_PI / 2.0
#define PI2 DirectX::XM_PI * 2.0f

using namespace DirectX;

Camera::Camera() :
	m_CameraBuffer { 0.0f, 0.25f, -1.0f, 1.0f },
	m_Heading(0.0f),
	m_Pitch(0.0f)
{}

void Camera::CreateResource(ID3D12Device11* device, DescriptorHeap* descriptorHeap)
{
	D3D12_HEAP_PROPERTIES heapProp = { };
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Alignment = 0;
	resDesc.Width = Align(sizeof(XMFLOAT4) * 4,256);
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&m_ConstantBuffer));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc = {};
	cbDesc.BufferLocation = m_ConstantBuffer->GetGPUVirtualAddress();
	cbDesc.SizeInBytes = resDesc.Width;
	D3D12_CPU_DESCRIPTOR_HANDLE cbHandle = descriptorHeap->GetCPUHandle(2);
	device->CreateConstantBufferView(&cbDesc, cbHandle);
	Update(0.0f);
}

void Camera::Update(float deltaTime)
{
	if (Input::GetButtonState(0))
	{
		float MouseSensitivity = 1.65f;
		m_Heading += Input::GetMouseXDelta() * MouseSensitivity;
		if (m_Heading > PI2)
			m_Heading -= PI2;
		if (m_Heading < 0)
			m_Heading += PI2;
		m_Pitch = clamp(m_Pitch + Input::GetMouseYDelta() * MouseSensitivity, MinPitch, MaxPitch);
	}

	XMMATRIX PitchYawMatrix = XMMatrixRotationRollPitchYaw((float)m_Pitch, (float)m_Heading, 0.0f);

	m_CameraBuffer.Forward = XMVector3Transform({ 0.0f, 0.0f, 1.0f, 0.0f }, PitchYawMatrix);
	m_CameraBuffer.Right = XMVector3Transform({ 1.0f, 0.0f, 0.0f, 0.0f }, PitchYawMatrix);
	m_CameraBuffer.Up = XMVector3Transform({ 0.0f, -1.0f, 0.0f, 0.0f }, PitchYawMatrix);

	XMVECTOR direction = {};
	if (Input::GetKeyState('W'))
	{
		direction += { 0.0f, 0.0f, 1.0f, 0.0f };
	}
	if (Input::GetKeyState('S'))
	{
		direction += { 0.0f, 0.0f, -1.0f, 0.0f };
	}
	if (Input::GetKeyState('A'))
	{
		direction += { -1.0f, 0.0f, 0.0f, 0.0f };
	}
	if (Input::GetKeyState('D'))
	{
		direction += { 1.0f, 0.0f, 0.0f, 0.0f };
	}
	if (Input::GetKeyState('E'))
	{
		direction += { 0.0f, 1.0f, 0.0f, 0.0f };
	}
	if (Input::GetKeyState('Q'))
	{
		direction += { 0.0f, -1.0f, 0.0f, 0.0f };
	}
	float MovementSensitivity = 2.0f;
	direction *= MovementSensitivity * deltaTime;
	direction = XMVector3Transform(direction, PitchYawMatrix);
	m_CameraBuffer.CameraPosition += direction;
	//XMMATRIX ViewMatrix = XMMatrixLookToLH(m_CameraPosition, Forward, Up);
	//XMMATRIX ProjectionMatrix = XMMatrixPerspectiveFovLH(m_FOV, m_AspectRatio, 0.01f, 25.0f);
	//XMMATRIX Matrix = ViewMatrix * ProjectionMatrix;
	Upload();
}

void Camera::Upload()
{
	void* Destination = nullptr;
	const void* Source = &m_CameraBuffer;
	const UINT Size = sizeof(m_CameraBuffer);
	const D3D12_RANGE readRange = { 0, 0 };
	ThrowIfFailed(m_ConstantBuffer->Map(0, &readRange, &Destination));
	memcpy(Destination, Source, Size);
	m_ConstantBuffer->Unmap(0, nullptr);
}