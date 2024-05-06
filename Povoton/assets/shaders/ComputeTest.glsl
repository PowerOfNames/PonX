#type compute
#version 460
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(std140, set = 0, binding = 0) uniform CameraUBO
{
	mat4 View;
	mat4 InverseView;
	mat4 Projection;
	mat4 ViewProjection;
	vec4 Forward;
	vec4 Position;
	float FOV;
}u_Camera;

layout(std140, set = 0, binding = 1) uniform RayMarchingUBO
{
    vec4 BackgroundColor;
    vec4 ResolutionTime;
    uint64_t ParticleCount;
}u_MetaData;

struct Particle {
    vec4 PositionRadius;
    vec4 Velocity;
    vec4 Color;
    uint64_t ID;
    uint64_t IDPad;
};

layout(std140, set = 1, binding = 0) readonly buffer ParticleSSBOIn
{
   Particle particlesIn[ ];
}ssbo_ParticlesIn;

layout(std140, set = 1, binding = 1) buffer ParticleSSBOOut
{
   Particle particlesOut[ ];
}ssbo_ParticlesOut;

//layout(set = 2, binding = 0, rgba8) uniform writeonly image2D DistanceField;

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

void main() 
{
    //uint index = gl_GlobalInvocationID.x;

    for(int index = 0; index < u_MetaData.ParticleCount; index++)
    {
        ssbo_ParticlesOut.particlesOut[index].PositionRadius.xyz = ssbo_ParticlesIn.particlesIn[index].PositionRadius.xyz + (ssbo_ParticlesIn.particlesIn[index].Velocity.xyz * u_MetaData.ResolutionTime.z) / 20.0;
        ssbo_ParticlesOut.particlesOut[index].PositionRadius.w = ssbo_ParticlesIn.particlesIn[index].PositionRadius.w;
        ssbo_ParticlesOut.particlesOut[index].Velocity.xyz = ssbo_ParticlesIn.particlesIn[index].Velocity.xyz * 1.0;
        ssbo_ParticlesOut.particlesOut[index].Color = ssbo_ParticlesIn.particlesIn[index].Color;
        ssbo_ParticlesOut.particlesOut[index].ID = ssbo_ParticlesIn.particlesIn[index].ID;
        ssbo_ParticlesOut.particlesOut[index].IDPad = ssbo_ParticlesIn.particlesIn[index].IDPad;
    } 
}