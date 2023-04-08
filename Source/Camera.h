#pragma once
#include "PCH.h"

class DescriptorHeap;

class Camera
{
public:
	Camera();
	void CreateResource(ID3D12Device11* device, DescriptorHeap* descriptorHeap);
	void Update(float deltaTime);
	void Upload();
private:
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_ConstantBuffer;
	struct CameraBuffer
	{
		DirectX::XMVECTOR CameraPosition;
		DirectX::XMVECTOR Forward;
		DirectX::XMVECTOR Right;
		DirectX::XMVECTOR Up;
	} m_CameraBuffer;
	float m_Heading;
	float m_Pitch;
};

