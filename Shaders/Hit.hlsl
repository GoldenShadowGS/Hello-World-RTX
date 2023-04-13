#include "Common.hlsl"

RaytracingAccelerationStructure Scene : register(t0);

struct TriVertex
{
	float4 normal;
	float4 color;
};

StructuredBuffer<TriVertex> vertex : register(t2);

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
	//payload.colorAndDistance = float4((1 - RayTCurrent() * 0.5f) * 0.5f, RayTCurrent() * 0.5f, 0, RayTCurrent());
	uint id = PrimitiveIndex();
	float epsilon = 0.0000001f;
	//float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

	float3 worldNormal = mul((float3x3)ObjectToWorld3x4(), vertex[id].normal.xyz);
	float3 worldRayOrigin = WorldRayOrigin() + (worldNormal * epsilon * 10.0f) + (WorldRayDirection() * RayTCurrent());

	float3 color = vertex[id].color.xyz;
	//payload.colorAndDistance.xyz = worldNormal;

	RayDesc ray;
	ray.Origin = worldRayOrigin;
	ray.Direction = reflect(WorldRayDirection(), worldNormal);
	ray.TMin = epsilon;
	ray.TMax = 100000;
	if (payload.depth < 30)
	{
		payload.depth += 1;
		TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
	}
	payload.colorAndDistance.xyz *= color;
	payload.colorAndDistance.w += RayTCurrent();

	//payload.colorAndDistance.xyz += float4(payload.colorAndDistance.xyz * color, RayTCurrent());
}
