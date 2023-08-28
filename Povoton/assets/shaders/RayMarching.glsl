#type vertex
#version 460
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
	
layout(location = 0) in vec3 a_Position;		
layout(location = 1) in vec4 a_Color;		
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexID;

layout(location = 0) out vec4 o_Color;

layout(std140, set = 0, binding = 0) uniform CameraData
{
	mat4 View;
	mat4 Projection;
	mat4 ViewProjection;
} u_Camera;

layout(std140, set = 0, binding = 1) uniform SceneData
{
	vec4 FogColor;
	vec4 FogDistance;
	vec4 AmbientColor;
	vec4 SunlightDirection;
	vec4 SunlightColor;
} u_Scene;


void main()
{
	o_Color = a_Color;
	gl_Position = vec4(a_Position, 1.0f);
}

#type fragment
#version 460
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

layout(location = 0) in vec4 v_Color;

layout(location = 0) out vec4 color;

layout(std140, set = 0, binding = 0) uniform CameraData
{
	mat4 ViewMatrix;
	mat4 ProjectionMatric;
	mat4 ViewProjectionMatrix;
} u_Camera;


layout(std140, set = 0, binding = 1) uniform SceneData
{
	vec4 FogColor;
	vec4 FogDistance;
	vec4 AmbientColor;
	vec4 SunlightDirection;
	vec4 SunlightColor;
} u_Scene;


struct Particle {
    vec2 Position;
    vec2 Velocity;
    vec4 Color;
    uint64_t ID;
};

layout(std140, set = 1, binding = 0) readonly buffer ParticlesIn {
   Particle particles[ ];
}ParticleIn;

//layout(set = 2, binding = 0) uniform sampler u_Sampler;
//layout(set = 2, binding = 1) uniform texture2D u_DistanceMaps[32];



struct Ray
{
	vec3 Origin;
	vec3 Direction;
	vec3 Position;
};

vec3 RayMarch(in Ray currentRay)
{
	
	return currentRay.Direction;
}

void main()
{
	Ray currentRay;
	currentRay.Origin = vec3(0.0, 0.0, 0.0);
	currentRay.Direction = vec3(0.0, 0.0, 1.0);
	currentRay.Position = vec3(0.0, 0.0, 0.0);

	//color = vec4(RayMarch(currentRay), 1.0);
	color = ParticleIn.particles[0].Color;
	//color = u_Scene.AmbientColor;
}