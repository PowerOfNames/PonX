#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 v_FragColor;
layout(location = 1) in vec2 v_TexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 2) uniform sampler2D t_TexSampler;

void main()
{
	outColor = texture(t_TexSampler, v_TexCoord);
}
