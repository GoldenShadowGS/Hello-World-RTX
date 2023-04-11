#pragma once
#include "PCH.h"
#include "StructuredBuffer.h"

class Camera
{
public:
	Camera();
	void CreateResource(ID3D12Device11* device, HeapManager* heap, DescriptorHeap* descriptorHeap);
	void Update(float deltaTime);
	inline void SetFOV(float fov) { m_FOV = fov; }
	inline void SetAspectRatio(float aspectRatio) { m_AspectRatio = aspectRatio; }
private:
	StructuredBuffer m_StructuredBuffer;
	struct CameraBuffer
	{
		DirectX::XMVECTOR CameraPosition;
		DirectX::XMVECTOR Forward;
		DirectX::XMVECTOR Right;
		DirectX::XMVECTOR Up;
	} m_CameraBuffer;
	float m_Heading;
	float m_Pitch;
	float m_FOV;
	float m_AspectRatio;
};

