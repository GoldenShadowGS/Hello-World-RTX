#include "Common.hlsl"

[shader("closesthit")] 
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
  payload.colorAndDistance = float4((1 - RayTCurrent() * 0.5f)* 0.5f, RayTCurrent()* 0.5f, 0, RayTCurrent());
}
