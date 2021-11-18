#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 v_FragColor;
layout(location = 1) in vec2 v_TexCoord;



layout(location = 0) out vec4 outColor;

layout(binding = 2) uniform sampler2D t_Texture;

layout(set = 0, binding = 1) uniform SceneData
{
	vec4 FogColor;
	vec4 FogDistance;
	vec4 AmbientColor;
	vec4 SunlightDirection;
	vec4 SunlightColor;
} sceneData;

void main()
{
	outColor = vec4(v_FragColor + sceneData.AmbientColor.xyz, 1.0) * texture(t_Texture, v_TexCoord);
}
