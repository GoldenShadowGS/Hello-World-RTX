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
	m_CameraBuffer { -1.0f, 0.0f, 0.5f, 1.0f },
	m_Heading(0.0f),
	m_Pitch(0.5f),
	m_FOV(1.25f),
	m_AspectRatio(1.0f)
{}

void Camera::CreateResource(ID3D12Device11* device, HeapManager* heap, DescriptorHeap* descriptorHeap)
{
	m_StructuredBuffer.CreateResource(device, heap, descriptorHeap, sizeof(XMFLOAT4) * 4, 2);
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

	XMMATRIX cameraLookMatrix = XMMatrixRotationY(m_Pitch) * XMMatrixRotationZ(m_Heading);
	m_CameraBuffer.Forward = XMVector3Transform({ m_FOV, 0.0, 0.0f, 0.0f }, cameraLookMatrix);
	m_CameraBuffer.Right = XMVector3Transform({ 0.0f, m_AspectRatio, 0.0f, 0.0f }, cameraLookMatrix);
	m_CameraBuffer.Up = XMVector3Transform({ 0.0f, 0.0f, -1.0f, 0.0f }, cameraLookMatrix);

	XMVECTOR direction = {};
	if (Input::GetKeyState('W'))
	{
		direction += { 1.0f, 0.0f, 0.0f, 0.0f };
	}
	if (Input::GetKeyState('S'))
	{
		direction += { -1.0f, 0.0f, 0.0f, 0.0f };
	}
	if (Input::GetKeyState('A'))
	{
		direction += { 0.0f, -1.0f, 0.0f, 0.0f };
	}
	if (Input::GetKeyState('D'))
	{
		direction += { 0.0f, 1.0f, 0.0f, 0.0f };
	}
	if (Input::GetKeyState('E'))
	{
		direction += { 0.0f, 0.0f, 1.0f, 0.0f };
	}
	if (Input::GetKeyState('Q'))
	{
		direction += { 0.0f, 0.0f, -1.0f, 0.0f };
	}
	float MovementSensitivity = 2.0f;
	direction *= MovementSensitivity * deltaTime;
	direction = XMVector3Transform(direction, cameraLookMatrix);
	m_CameraBuffer.CameraPosition += direction;
	m_StructuredBuffer.Upload(&m_CameraBuffer, sizeof(m_CameraBuffer));
}

//void Camera::Upload()
//{
//	void* Destination = nullptr;
//	const void* Source = &m_CameraBuffer;
//	const UINT Size = sizeof(m_CameraBuffer);
//	const D3D12_RANGE readRange = { 0, 0 };
//	ThrowIfFailed(m_Resource->Map(0, &readRange, &Destination));
//	memcpy(Destination, Source, Size);
//	m_Resource->Unmap(0, nullptr);
//}