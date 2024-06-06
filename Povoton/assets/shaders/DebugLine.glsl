#type vertex
#version 460
#extension GL_ARB_shader_draw_parameters : require
	
layout(location = 0) in vec3 a_Position;		
layout(location = 1) in vec4 a_Color;

layout(std140, set = 0, binding = 0) uniform BillboardCameraUBO
{
	mat4 View;
	mat4 InverseView;
	mat4 Projection;
	mat4 ViewProjection;
  mat4 InverseViewProjection;
	vec4 Forward;
	vec4 Position;
	float FOV;
}u_BillboardCamera;

layout(std140, set = 0, binding = 1) uniform ActiveCameraUBO
{
	mat4 View;
	mat4 InverseView;
	mat4 Projection;
	mat4 ViewProjection;
	mat4 InverseViewProjection;
	vec4 Forward;
	vec4 Position;
	float FOV;
} u_ActiveCamera;

layout(location = 0) out vec4 o_Color;

void main()
{	
	o_Color = a_Color;

	gl_Position = u_ActiveCamera.ViewProjection * vec4(a_Position, 1.0f);
}



#type fragment
#version 460
#extension GL_ARB_shader_draw_parameters : require

layout(location = 0) out vec4 color;

layout(location = 0) in vec4 v_Color;

void main()
{
	color = v_Color;
}