#version 450 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec3 inBitangent;
layout (location = 4) in vec2 inUv;

layout (location = 0) out vec3 outPosition;
layout (location = 1) out vec3 outNormal;

layout (binding = 0) uniform ModelViewProjection
{
    mat4 model;
    mat4 view;
    mat4 proj;
} mvp;

void main()
{
    outPosition = (mvp.model * vec4(inPosition, 1.0)).rgb;
    gl_Position = mvp.proj * mvp.view * mvp.model * vec4(inPosition, 1.0);

    // TODO: Move inverse model to the cpu.
    outNormal = mat3(transpose(inverse(mvp.model))) * normalize(inNormal);
}
