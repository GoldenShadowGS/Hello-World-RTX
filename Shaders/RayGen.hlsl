#include "Common.hlsl"

// Raytracing output texture, accessed as a UAV
RWTexture2D< float4 > gOutput : register(u0);

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0);

struct Camera
{
	float4 position;
	float4 direction;
	float4 right;
	float4 up;
};

StructuredBuffer<Camera> cam : register (t1, space0);

float4 GLobalLight = normalize(float4(1,1,1,0));

[shader("raygeneration")]
void RayGen()
{
	// Initialize the ray payload
	HitInfo payload;
	payload.colorAndDistance = float4(0.9, 0.6, 0.2, 1);

	uint2 launchIndex = DispatchRaysIndex().xy;
	float2 dims = float2(DispatchRaysDimensions().xy);
	float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);

	// Get the location within the dispatched 2D grid of work items
	// (often maps to pixels, so this could represent a pixel coordinate).

	RayDesc ray;
	ray.Origin = cam[0].position.xyz;
	float3 up = cam[0].up.xyz * d.y;
	float3 right = cam[0].right.xyz * d.x;
	ray.Direction = normalize(cam[0].direction.xyz + up + right);
	ray.TMin = 0;
	ray.TMax = 100000;

	// Trace the ray
	TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);

	gOutput[launchIndex] = float4(payload.colorAndDistance.rgb, 1.f);
}
