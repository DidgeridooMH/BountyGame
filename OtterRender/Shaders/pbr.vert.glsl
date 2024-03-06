#version 450 core

layout (location = 0) in vec3 inPosition;

layout (location = 0) out vec3 outPosition;

void main()
{
    gl_Position = vec4(inPosition, 1.0);
    outPosition = inPosition;
}