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

float toSRGB(float x)
{
	if(x <= 0.0031308)
	{
		return x*12.92f;
	}
	else
	{
		return pow(1.055f*x, 1/2.4f) - 0.055f;
	}
}

float toLinear(float x)
{
	if(x <= 0.04045)
	{
		return x/12.92f;
	}
	else
	{
		return pow((x+0.055)/1.055, 2.4f);
	}
}


void main()
{
	vec4 texColor = texture(t_Texture, v_TexCoord);
	outColor = vec4(toSRGB(texColor.x), toSRGB(texColor.y), toSRGB(texColor.z), texColor.a);
	//outColor = texColor;
}
