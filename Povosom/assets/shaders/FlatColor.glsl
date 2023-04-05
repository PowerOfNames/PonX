#type vertex
#version 460
	
layout(location = 0) in vec3 a_Position;		
layout(location = 1) in vec4 a_Color;		
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexID;
layout(location = 4) in int a_EntityID;	


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
layout(location = 2) out flat int o_EntityID;


void main()
{
	Output.Color = a_Color;
	Output.TexCoord = a_TexCoord;
	o_EntityID = a_EntityID;

	gl_Position = u_Camera.ViewProjection * vec4(a_Position, 1.0f);
}

#type fragment
#version 460
	
layout(location = 0) out vec4 color;
layout(location = 1) out int entityID;

struct VertexInput
{
	vec4 Color;
	vec2 TexCoord;
};

layout(location = 0) in VertexInput Input;
layout(location = 2) in flat int v_EntityID;

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

void main()
{
	vec3 resultColor = Input.Color.xyz * (u_Scene.AmbientColor.xyz * u_Scene.AmbientColor.a);
	color = vec4(resultColor, Input.Color.a);
	entityID = v_EntityID;
}
