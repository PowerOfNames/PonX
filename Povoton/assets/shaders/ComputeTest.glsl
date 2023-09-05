#type compute
#version 460
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable


layout (set = 0, binding = 0) uniform ParameterUBO {
    float DeltaTime;
} ubo;

struct Particle {
    vec4 PositionRadius;
    vec3 Velocity;
    vec3 Color;
    uint64_t ID;
};

layout(std140, set = 1, binding = 0) readonly buffer ParticleSSBOIn {
   Particle particlesIn[ ];
}ParticleIn;

layout(std140, set = 1, binding = 1) buffer ParticleSSBOOut {
   Particle particlesOut[ ];
};

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main() 
{
    uint index = gl_GlobalInvocationID.x;  

    Particle particleIn = ParticleIn.particlesIn[index];

    particlesOut[index].PositionRadius.xyz = particleIn.PositionRadius.xyz + particleIn.Velocity.xyz * ubo.DeltaTime;
    particlesOut[index].PositionRadius.w = particleIn.PositionRadius.w;
    particlesOut[index].Velocity = particleIn.Velocity;    
}