#type compute
#version 460
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

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
};

layout(std140, set = 1, binding = 0) readonly buffer ParticleSSBOVertex_0
{
   Particle particlesIn[ ];
}ssbo_ParticleVertexIn;

layout(std140, set = 1, binding = 1) buffer ParticleSSBOVertex_1
{
   Particle particlesOut[ ];
}ssbo_ParticleVertexOut;

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main() 
{
    int index = int(gl_GlobalInvocationID.x) + int(gl_GlobalInvocationID.y) * int(gl_NumWorkGroups.x) + int(gl_GlobalInvocationID.z) * int(gl_NumWorkGroups.x * gl_NumWorkGroups.y);

//    for(int index = 0; index < u_MetaData.ParticleCount; index++)
//    {
//        ssbo_ParticleVertexOut.particlesOut[index].PositionRadius.xyz = ssbo_ParticleVertexIn.particlesIn[index].PositionRadius.xyz + (ssbo_ParticleVertexIn.particlesIn[index].Velocity.xyz * u_MetaData.ResolutionTime.z) / 20.0;
//        ssbo_ParticleVertexOut.particlesOut[index].PositionRadius.w = ssbo_ParticleVertexIn.particlesIn[index].PositionRadius.w;
//        ssbo_ParticleVertexOut.particlesOut[index].Velocity.xyz = ssbo_ParticleVertexIn.particlesIn[index].Velocity.xyz * 1.0;
//        ssbo_ParticleVertexOut.particlesOut[index].Color = ssbo_ParticleVertexIn.particlesIn[index].Color;
//    }
    if(index < u_MetaData.ParticleCount)
    {
	    ssbo_ParticleVertexOut.particlesOut[index].PositionRadius.xyz = ssbo_ParticleVertexIn.particlesIn[index].PositionRadius.xyz + (ssbo_ParticleVertexIn.particlesIn[index].Velocity.xyz * u_MetaData.ResolutionTime.z) / 20.0;
        ssbo_ParticleVertexOut.particlesOut[index].PositionRadius.w = ssbo_ParticleVertexIn.particlesIn[index].PositionRadius.w;
        ssbo_ParticleVertexOut.particlesOut[index].Velocity.xyz = ssbo_ParticleVertexIn.particlesIn[index].Velocity.xyz * 1.0;
        ssbo_ParticleVertexOut.particlesOut[index].Color = ssbo_ParticleVertexIn.particlesIn[index].Color;
    }
}