#type compute
#version 460
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

layout(std140, set = 0, binding = 0) uniform CameraData
{
	mat4 View;
	mat4 Projection;
	mat4 ViewProjection;
	vec4 Forward;
	vec4 Position;
	float FOV;
} u_Camera;

//layout (set = 0, binding = 1) uniform ParameterUBO {
//    float DeltaTime;
//} ubo;

struct Particle {
    vec4 PositionRadius;
    vec4 Velocity;
    vec4 Color;
    uint64_t ID;
    uint64_t IDPad;
};

layout(std140, set = 1, binding = 0) readonly buffer ParticlesIn {
   Particle particlesIn[ ];
}ParticleIn;

/*layout(std140, set = 1, binding = 1) buffer ParticleSSBOOut {
   Particle particlesOut[ ];
};*/

//layout(set = 2, binding = 0, rgba8) uniform writeonly image2D DistanceField;

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main() 
{
    uint index = gl_GlobalInvocationID.x;  

    Particle particleIn = ParticleIn.particlesIn[index];

    //particlesOut[index].PositionRadius.xyz = particleIn.PositionRadius.xyz + particleIn.Velocity.xyz * ubo.DeltaTime;
    //particlesOut[index].PositionRadius.w = particleIn.PositionRadius.w;
    //particlesOut[index].Velocity = particleIn.Velocity;    
}