#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "rt.common.glsl"

layout(location = 0) rayPayloadInEXT HitPayload payload;

void main()
{
  payload.distance = -1;
}
