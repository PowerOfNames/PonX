#type vertex
#version 460
#extension GL_ARB_shader_draw_parameters : require
	
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
layout(location = 3) out flat float o_TexID;


void main()
{
	Output.Color = a_Color;
	Output.TexCoord = a_TexCoord;
	o_EntityID = a_EntityID;
	o_TexID = a_TexID;

	gl_Position = u_Camera.ViewProjection * vec4(a_Position, 1.0f);
}

#type fragment
#version 460
#extension GL_ARB_shader_draw_parameters : require

layout(location = 0) out vec4 color;
layout(location = 1) out int entityID;

struct VertexInput
{
	vec4 Color;
	vec2 TexCoord;
};

layout(location = 0) in VertexInput Input;
layout(location = 2) in flat int v_EntityID;
layout(location = 3) in flat float v_TexID;

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
	/*switch(int(Input.TexID))
	{
		case 0: texColor *= texture(sampler2D(u_Textures[0], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 1: texColor *= texture(sampler2D(u_Textures[1], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 2: texColor *= texture(sampler2D(u_Textures[2], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 3: texColor *= texture(sampler2D(u_Textures[3], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 4: texColor *= texture(sampler2D(u_Textures[4], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 5: texColor *= texture(sampler2D(u_Textures[5], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 6: texColor *= texture(sampler2D(u_Textures[6], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 7: texColor *= texture(sampler2D(u_Textures[7], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 8: texColor *= texture(sampler2D(u_Textures[8], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 9: texColor *= texture(sampler2D(u_Textures[9], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 10: texColor *= texture(sampler2D(u_Textures[10], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 11: texColor *= texture(sampler2D(u_Textures[11], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 12: texColor *= texture(sampler2D(u_Textures[12], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 13: texColor *= texture(sampler2D(u_Textures[13], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 14: texColor *= texture(sampler2D(u_Textures[14], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 15: texColor *= texture(sampler2D(u_Textures[15], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 16: texColor *= texture(sampler2D(u_Textures[16], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 17: texColor *= texture(sampler2D(u_Textures[17], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 18: texColor *= texture(sampler2D(u_Textures[18], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 19: texColor *= texture(sampler2D(u_Textures[19], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 20: texColor *= texture(sampler2D(u_Textures[20], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 21: texColor *= texture(sampler2D(u_Textures[21], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 22: texColor *= texture(sampler2D(u_Textures[22], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 23: texColor *= texture(sampler2D(u_Textures[23], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 24: texColor *= texture(sampler2D(u_Textures[24], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 25: texColor *= texture(sampler2D(u_Textures[25], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 26: texColor *= texture(sampler2D(u_Textures[26], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 27: texColor *= texture(sampler2D(u_Textures[27], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 28: texColor *= texture(sampler2D(u_Textures[28], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 29: texColor *= texture(sampler2D(u_Textures[29], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 30: texColor *= texture(sampler2D(u_Textures[30], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
		case 31: texColor *= texture(sampler2D(u_Textures[31], u_Sampler), Input.TexCoord * Input.TilingFactor); break;
	}*/

	
	//texColor *= texture(u_Texture, Input.TexCoord * Input.TilingFactor);
	
	color = vec4(texColor.xyz + u_Scene.AmbientColor.xyz, texColor.a);	
	entityID = v_EntityID;
}
