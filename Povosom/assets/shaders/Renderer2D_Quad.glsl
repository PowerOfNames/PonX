#type vertex
#version 460
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
	
layout(location = 0) in vec3 a_Position;		
layout(location = 1) in vec4 a_Color;		
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexID;


layout(std140, set = 0, binding = 0) uniform Camera
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

struct ObjectData
{
	//mat4 ModelMatrix;
	uint TexID;
	float TilingFactor;
};
layout(std140, set = 1, binding = 0) readonly buffer Objects
{
	ObjectData objects[];
}b_Objects;

struct VertexOutput
{
	vec4 Color;
	vec2 TexCoord;
};

layout(location = 0) out VertexOutput Output;
layout(location = 2) out flat float o_TexID;


void main()
{
	Output.Color = a_Color;
	Output.TexCoord = a_TexCoord;
	o_TexID = a_TexID;

	gl_Position = u_Camera.ViewProjection * vec4(a_Position, 1.0f);
}

#type fragment
#version 460
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

layout(location = 0) out vec4 color;

struct VertexInput
{
	vec4 Color;
	vec2 TexCoord;
};

layout(location = 0) in VertexInput Input;
layout(location = 2) in flat float v_TexID;

layout(std140, set = 0, binding = 0) uniform Camera
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


struct ObjectData
{
	//mat4 ModelMatrix;
	uint TexID;
	float TilingFactor;
};
layout(std140, set = 1, binding = 0) readonly buffer Objects
{
	ObjectData objects[];
}b_Objects;

//layout(binding = 0) uniform sampler2D u_Textures[32];
layout(set = 2, binding = 0) uniform sampler u_Sampler;
layout(set = 2, binding = 1) uniform texture2D u_Textures[32];

void main()
{
	vec4 texColor = Input.Color;
	texColor *= texture(sampler2D(u_Textures[int(v_TexID)], u_Sampler), Input.TexCoord * 1.0f);	
	color = vec4(texColor.rgb + u_Scene.AmbientColor.rgb * u_Scene.AmbientColor.a, texColor.a);	
	
	
	//entityID = v_EntityID;
}
