#include "Common.hlsl"

[shader("miss")]
void Miss(inout HitInfo payload : SV_RayPayload)
{
    // Find out how to get ray direction
    float3 color = (WorldRayDirection() + 1.0f) / 2.0f;
    payload.colorAndDistance = float4(color, 100000);

}