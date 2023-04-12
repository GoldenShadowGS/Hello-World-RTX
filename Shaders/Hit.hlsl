#include "Common.hlsl"

RaytracingAccelerationStructure Scene : register(t0);

struct TriVertex
{
	float3 normal;
	float pad;
	float3 color;
	float pad2;
};

StructuredBuffer<TriVertex> vertex : register(t2);

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
	//payload.colorAndDistance = float4((1 - RayTCurrent() * 0.5f) * 0.5f, RayTCurrent() * 0.5f, 0, RayTCurrent());
	uint id = PrimitiveIndex();
	float epsilon = 0.0000001f;
	//float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

	float3 worldNormal = normalize(mul((float3x3)ObjectToWorld3x4(), vertex[id].normal));
	float3 worldRayOrigin = WorldRayOrigin() + (worldNormal * epsilon * 10.0f) + (WorldRayDirection() * RayTCurrent());
	//worldNormal = (worldNormal * 2) - 1;
	//float3 originalcolor = payload.colorAndDistance.xyz;
	//float3 combinedcolor = (originalcolor + barycentrics);
	//float3 color = vertex[id].color.xyz;
	float3 color = { 0.95f, 0.9f, 0.5f };
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
