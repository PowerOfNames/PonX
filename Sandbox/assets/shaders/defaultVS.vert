#version 460
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Color;
layout (location = 2) in vec2 TexCoord;

layout(location = 0) out vec3 v_FragColor;
layout(location = 1) out vec2 v_TexCoord;

layout(binding = 0) uniform UniformBufferObject
{
	mat4 ViewMatrix;
	mat4 ProjectionMatrix;
	mat4 ViewProjMatrix;
} ubo;

struct ObjectData
{
	mat4 ModelMatrix;
}

layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer
{
	ObjectData objects[];
} objectBuffer;

layout( push_constant ) uniform PushConstants
{
	mat4 RenderMatrix;
} constant;


void main()
{
	gl_Position = ubo.ViewProjMatrix * objectBuffer.objects[gl_BaseInstance].ModelMatrix * vec4(Position, 1.0);
	v_FragColor = Color;
	v_TexCoord = TexCoord;
}
