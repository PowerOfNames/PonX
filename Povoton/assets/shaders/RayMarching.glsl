#type vertex
#version 460
#extension GL_ARB_shader_draw_parameters : enable
	
layout(location = 0) in vec3 a_Position;		
layout(location = 1) in vec4 a_Color;		
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexID;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out vec2 o_UV;

void main()
{
	o_Color = a_Color;
	o_UV = a_TexCoord;
	gl_Position = vec4(a_Position, 1.0f);
}

#type fragment
#version 460
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_UV;

layout(location = 0) out vec4 finalColor;

layout(std140, set = 0, binding = 0) uniform CameraUBO
{
	mat4 View;
	mat4 Projection;
	mat4 ViewProjection;
	vec4 Forward;
	vec4 Position;
	float FOV;
} u_Camera;


layout(set = 0, binding = 1) uniform RayMarchingUBO
{
	vec4 BackgroundColor;
	vec4 ResolutionTime;
	uint64_t ParticleCount;
} u_RayMarching;


struct Particle {
    vec4 PositionRadius;
    vec4 Velocity;
    vec4 Color;
    uint64_t ID;
    uint64_t IDPad;
};

layout(std140, set = 1, binding = 0) readonly buffer ParticleSSBOIn {
   Particle particles[ ];
}particlesIn;

//layout(set = 2, binding = 0) uniform sampler u_Sampler;
//layout(set = 2, binding = 1) uniform texture2D u_DistanceMaps[32];
layout(set = 2, binding = 0) uniform sampler2D DistanceField;

#define PI 3.14159
#define epsilon 0.000000001

struct Ray
{
	vec3 Origin;
	vec3 Direction;
	vec3 Position;
};


float SphereSDF(in vec3 rayPos, in vec3 spherePos, in float sphereRadius)
{
	float time = u_RayMarching.ResolutionTime.z * 1.6f;
	float displacement = sin(time + 7.0 * rayPos.x) * -cos(time + 7.0 * rayPos.y) * cos(time + 7.0 * rayPos.z) * 0.1 -0.5;
	float dist = abs(length(rayPos-spherePos)) - sphereRadius;

	return dist + displacement;
}

vec3 CalculateSurfaceNormal(in vec3 hitPos, in vec4 particleDims)
{
	vec3 offset = vec3(0.01, 0.0, 0.0);
	
	
	float gradient_x = SphereSDF(hitPos + offset.xyy, particleDims.xyz, particleDims.w) - SphereSDF(hitPos - offset.xyy, particleDims.xyz, particleDims.w);
	float gradient_y = SphereSDF(hitPos + offset.yxy, particleDims.xyz, particleDims.w) - SphereSDF(hitPos - offset.yxy, particleDims.xyz, particleDims.w);
	float gradient_z = SphereSDF(hitPos + offset.yyx, particleDims.xyz, particleDims.w) - SphereSDF(hitPos - offset.yyx, particleDims.xyz, particleDims.w);

	return normalize(vec3(gradient_x, gradient_y, gradient_z));
}

vec3 Phongg(in vec3 viewDir, in vec3 hitPos, in vec3 lightPos, in vec3 normal, in vec4 ambient, in vec2 specular, in vec4 diffuse)
{
	vec3 dirToLight = normalize(lightPos - hitPos);
	vec3 reflectionVector = 2.0 * dot(normal, dirToLight) * normal - dirToLight;

	vec3 ambientLight = ambient.rgb * ambient.w;
	vec3 diffuseLight = diffuse.rgb * dot(normal, -dirToLight) * diffuse.w;
	float specularLight = specular.x * (2*specular.y)/(2*PI) * pow(max(0.0, dot(viewDir, reflectionVector)), specular.y); 

	return (ambientLight + diffuseLight + specularLight) * diffuse.rgb;
}

const float MAX_STEPS = 128;
const float HIT_DISTANCE = 0.001;
const float MAX_DISTANCE = 1000.0;

const vec2 SPECULAR = vec2(2.0, 2.0);
const vec3 LIGHT = vec3(0.0, 5.0, 0.0);

vec3 RayMarch(in Ray currentRay)
{
	Particle nearestParticle;
	float shortestDist = 10000.0;
	float totalDistanceTraveled = 0.0;
	for(int i = 0; i < MAX_STEPS; i++)
	{
		for(int particleID = 0; particleID < u_RayMarching.ParticleCount; particleID++)
		{
			Particle currentParticle = particlesIn.particles[particleID];
			float currentDist = SphereSDF(currentRay.Position, currentParticle.PositionRadius.xyz, currentParticle.PositionRadius.w);
			
			if(currentDist < shortestDist)
			{
				shortestDist = currentDist;
				nearestParticle = currentParticle;

				if(currentDist <= HIT_DISTANCE)
				{
					vec3 normal = CalculateSurfaceNormal(currentRay.Position, nearestParticle.PositionRadius);
					return Phongg(currentRay.Direction, currentRay.Position, LIGHT, normal, vec4(u_RayMarching.BackgroundColor.rgb, 1.0), SPECULAR, vec4(nearestParticle.Color.rgb, 0.3));			
				}
			}
		}	
		
		totalDistanceTraveled += shortestDist;
		if(totalDistanceTraveled > MAX_DISTANCE)
			break;
		
		currentRay.Position = currentRay.Position + currentRay.Direction * shortestDist;
	}

	return u_RayMarching.BackgroundColor.rgb;
}



vec3 CalculateDirection(in vec3 camPos, in vec2 uv, in vec3 cameraForward, in float fov, in vec2 resolution)
{
	vec2 correctedUV = 2.0 * uv - 1.0;
	vec3 right = normalize(cross(cameraForward, vec3(0.0, 1.0, 0.0)));
	vec3 up = normalize(cross(cameraForward, right));
	float aspectRatio = resolution.x / resolution.y;

	return normalize(cameraForward * (tan(fov*PI/360) * aspectRatio) + correctedUV.x * aspectRatio * right + correctedUV.y * up);
}



void main()
{
	Ray currentRay;
	currentRay.Origin = u_Camera.Position.xyz;
	currentRay.Position = currentRay.Origin;
	currentRay.Direction = CalculateDirection(currentRay.Origin, v_UV, u_Camera.Forward.xyz, u_Camera.FOV, u_RayMarching.ResolutionTime.xy);
	
	//finalColor = vec4(SphereSDF, 1.0);
	finalColor = vec4(RayMarch(currentRay), 1.0);
	//finalColor = vec4(1.0);
}