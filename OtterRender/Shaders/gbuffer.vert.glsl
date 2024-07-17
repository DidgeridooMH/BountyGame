#version 450 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inTangent;
layout (location = 3) in vec2 inUv;

layout (location = 0) out vec3 outPosition;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUv;
layout (location = 3) out mat3 outTBN;

layout (binding = 0) uniform ModelViewProjection
{
    mat4 view;
    mat4 proj;
} vp;

layout (push_constant) uniform Model
{
    mat4 model;
} model;

void main()
{
    outPosition = (model.model * vec4(inPosition, 1.0)).rgb;
    gl_Position = vp.proj * vp.view * model.model * vec4(inPosition, 1.0);

    // TODO: Move inverse model to the cpu.
    mat3 normalMatrix = transpose(inverse(mat3(model.model)));
    outNormal = normalMatrix * inNormal;

    vec3 T = normalize(inTangent.xyz - inNormal * dot(inTangent.xyz, inNormal));
    vec3 N = normalize(vec3(model.model * vec4(inNormal, 0.0)));
    T = normalize(vec3(model.model * vec4(inTangent.xyz, 0.0)));
    vec3 B = cross(N, T) * inTangent.w;
    outTBN = mat3(T, B, N);

    outUv = inUv;
}
