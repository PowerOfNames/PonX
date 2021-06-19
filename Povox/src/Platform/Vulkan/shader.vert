#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 Position;
layout (location = 1) in vec3 Color;
layout (location = 2) in vec2 TexCoord;

layout(location = 0) out vec3 v_FragColor;
layout(location = 1) out vec2 v_TexCoord;

layout(binding = 0) uniform UniformBufferObject
{
	mat4 ModelMatrix;
	mat4 ViewMatrix;
	mat4 ProjectionMatrix;
} ubo;


void main()
{
	gl_Position = ubo.ProjectionMatrix * ubo.ViewMatrix * ubo.ModelMatrix * vec4(Position, 0.0, 1.0);
	v_FragColor = Color;
	v_TexCoord = TexCoord;
}