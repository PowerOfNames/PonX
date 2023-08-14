#type compute
#version 460
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable


layout (set = 0, binding = 0) uniform ParameterUBO {
    float DeltaTime;
} ubo;

struct Particle {
    vec2 Position;
    vec2 Velocity;
    vec4 Color;
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

    particlesOut[index].Position = particleIn.Position + particleIn.Velocity.xy * ubo.DeltaTime;
    particlesOut[index].Velocity = particleIn.Velocity;    
}