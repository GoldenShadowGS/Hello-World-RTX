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
	//float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

	float3 worldRayOrigin = WorldRayOrigin() + (WorldRayDirection() * RayTCurrent());
	float3 worldNormal = mul(normalize(vertex[id].normal), (float3x3)ObjectToWorld3x4());

	//float3 originalcolor = payload.colorAndDistance.xyz;
	//float3 combinedcolor = (originalcolor + barycentrics);
	float3 color = vertex[id].color.xyz;

	RayDesc ray;
	ray.Origin = worldRayOrigin;
	ray.Direction = reflect(WorldRayDirection(), worldNormal);
	ray.TMin = 0;
	ray.TMax = 100000;

	if (payload.depth < 3)
	{
		payload.depth += 1;
		TraceRay(Scene, RAY_FLAG_NONE, 0, 0, 0, 0, ray, payload);
		payload.colorAndDistance.xyz += color;
	}
	//payload.colorAndDistance.xyz += float4(payload.colorAndDistance.xyz * color, RayTCurrent());
}
