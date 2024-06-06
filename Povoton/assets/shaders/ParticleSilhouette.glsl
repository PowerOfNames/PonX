#type vertex
#version 460
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(location = 0) in vec4 a_PositionRadius;
layout(location = 1) in vec4 a_Velocity;
layout(location = 2) in vec4 a_Color;

layout(location = 0) out VertexData {
    flat vec3 Position;
    flat float Radius;
    flat vec4 Color;
} Vertex;

void main()
{
    Vertex.Position = a_PositionRadius.xyz;
    Vertex.Radius = a_PositionRadius.a;
    Vertex.Color = a_Color;

    gl_Position = vec4(a_PositionRadius.xyz, 1.0);
}


#type geometry
#version 460

layout(points) in;

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
} u_BillboardCamera;

layout(std140, set = 0, binding = 1) uniform ActiveCameraUBO
{
	mat4 View;
	mat4 InverseView;
	mat4 Projection;
	mat4 ViewProjection;
    mat4 InverseViewProjection;
	vec4 Forward;
	vec4 Position;
	float FOV;
} u_ActiveCamera;


layout(location = 0) in VertexDataIn {
    vec3 Position;
    float Radius; // .x = radius
    vec4 Color;
} VertexIn[];

layout(triangle_strip, max_vertices=4) out;

// dont need Normal, because the 2D silhouette has no real normals we need to use, its just a tool to create the spherical partical
// normal calculation takes place in the fragment shader while raytracing the silhouettes
layout(location = 0) out SilhouetteData {
    vec3 Position;
    flat float Radius;
    flat vec4 Color;
    vec2 TexCoord;
    flat vec3 Center;
} SilhouetteCorner;

const float eps = 0.0000000001;

// Function that calculates the silhouette center and the radius of the silhouette, from Camera.Position, VertexIn.Position
// and VertexIn.Properties.x = ParticleRadius with
// sr -> silhouette radius
// sc -> silhouette center
// e  -> distance between VertexIn.Position and Camera.Position
// V  -> Vector from Camera.Position to silhouette border
void CalculateSilhouette(in vec3 particlePos, in vec3 cameraPos, in float particleRadius, inout vec3 sc, inout float sr) {
    // distance between particle position and camera position
    float e = max(eps, abs(distance(particlePos, cameraPos)));
    float rr = (particleRadius * particleRadius);
    // distance between particle position  and silhouette center
    float m =  rr / e;

    vec3 distCenter = (m / e) * (cameraPos - particlePos);
    sc = particlePos + distCenter;
    sr = sqrt(rr * (1 - (rr / (e * e))));
}
void main() {

    vec3 pos = gl_in[0].gl_Position.xyz;

    vec3 sc = vec3(0.0, 0.0, 0.0);
    float sr = 0.0;
    CalculateSilhouette(pos, u_BillboardCamera.Position.xyz, VertexIn[0].Radius, sc, sr);

    vec3 up = u_BillboardCamera.InverseView[1].xyz;
    vec3 right = u_BillboardCamera.InverseView[0].xyz;
    vec3 cornerPos = vec3(1.0);
    vec4 unnormPos = vec4(1.0);

    vec3 rMulUp = sr * up;
    vec3 rMulRight = sr * right;

    // calculate corner in World space
    cornerPos = sc + rMulUp + rMulRight;
    // bring corner to view space
    unnormPos = u_ActiveCamera.ViewProjection * vec4(cornerPos, 1.0);

    // bring corner to clip space
    gl_Position = vec4(unnormPos.xyz / unnormPos.w, 1.0);
    SilhouetteCorner.Position = cornerPos;
    SilhouetteCorner.Radius = VertexIn[0].Radius;
    SilhouetteCorner.TexCoord = vec2(1.0, 1.0);
    SilhouetteCorner.Center = VertexIn[0].Position;
    SilhouetteCorner.Color = VertexIn[0].Color;
    EmitVertex();


    cornerPos = sc + rMulUp - rMulRight;
    unnormPos = u_ActiveCamera.ViewProjection * vec4(cornerPos, 1.0);

    gl_Position = vec4(unnormPos.xyz / unnormPos.w, 1.0);
    SilhouetteCorner.Position = cornerPos;
    SilhouetteCorner.Radius = VertexIn[0].Radius;
    SilhouetteCorner.TexCoord = vec2(1.0, -1.0);
    SilhouetteCorner.Center = VertexIn[0].Position;
    SilhouetteCorner.Color = VertexIn[0].Color;
    EmitVertex();


    cornerPos = sc - rMulUp + rMulRight;
    unnormPos = u_ActiveCamera.ViewProjection * vec4(cornerPos, 1.0);

    gl_Position = vec4(unnormPos.xyz / unnormPos.w, 1.0);
    SilhouetteCorner.Position = cornerPos;
    SilhouetteCorner.Radius = VertexIn[0].Radius;
    SilhouetteCorner.TexCoord = vec2(-1.0, 1.0);
    SilhouetteCorner.Center = VertexIn[0].Position;
    SilhouetteCorner.Color = VertexIn[0].Color;
    EmitVertex();


    cornerPos = sc - rMulUp - rMulRight;
    unnormPos = u_ActiveCamera.ViewProjection * vec4(cornerPos, 1.0);

    gl_Position = vec4(unnormPos.xyz / unnormPos.w, 1.0);
    SilhouetteCorner.Position = cornerPos;
    SilhouetteCorner.Radius = VertexIn[0].Radius;
    SilhouetteCorner.TexCoord = vec2(-1.0, -1.0);
    SilhouetteCorner.Center = VertexIn[0].Position;
    SilhouetteCorner.Color = VertexIn[0].Color;
    EmitVertex();

    EndPrimitive();
}


#type fragment
#version 460

layout(std140, set = 0, binding = 1) uniform ActiveCameraUBO
{
	mat4 View;
	mat4 InverseView;
	mat4 Projection;
	mat4 ViewProjection;
    mat4 InverseViewProjection;
	vec4 Forward;
	vec4 Position;
	float FOV;
} u_ActiveCamera;

layout(location = 0) in SilhouetteData {
    vec3 Position;
    flat float Radius;
    flat vec4 Color;
    vec2 TexCoord;
    flat vec3 Center;
} SilhouetteCorner;

layout(location = 0) out vec4 FragColor;

/*
The rasterizer interpolates the uv coordinates
this way, the function can calculate a ray between the eye and the frag pos (which is the fragment position at the billboard)
and together with the center of the actual particle and its radius calculate the position, where the ray would hit the sphere

The algorithm was produced by Prof. Gumhold / Tu-Dresden
Pages 26-29: https://tu-dresden.de/ing/informatik/smt/cgv/ressourcen/dateien/lehre/ss-2020/scivis/02_SciVis_Particles.pdf?lang=en (11.2021)
*/
vec3 RaySphereIntersection(in vec3 eye, in vec3 fragPos, in vec3 center, in float radius) {
    float beta = (radius * sqrt(1 - length(SilhouetteCorner.TexCoord) * length(SilhouetteCorner.TexCoord))) / length(eye - center);
    float lambda = 1 / (1 + beta);
    return eye + lambda * (fragPos - eye);
}

void main() {
    //First: Determine if the pixel lies within the radius of the circle, if not, discard
    if(!(length(SilhouetteCorner.TexCoord) * length(SilhouetteCorner.TexCoord) <= 1))
    {
        //FragColor = vec4(1.0, 0.2, 0.5, 0.5);
        discard;
    }
    //Second: Color pixel according to lighting (normal calculation + light parameters from scenery's lighting system
    else
    {
        vec3 intersection = RaySphereIntersection(u_ActiveCamera.Position.xyz, SilhouetteCorner.Position, SilhouetteCorner.Center, SilhouetteCorner.Radius);
        vec3 normal = normalize((intersection - SilhouetteCorner.Center));

        //recalculate the depth buffer (quad fragment position -> sphere fragment position), by applying the view projection matrix and bringing it to clip space
        vec4 intersectionVP = u_ActiveCamera.ViewProjection * vec4(intersection, 1.0);
        float depth = ((intersectionVP.z / intersectionVP.w) + 1.0) / 2.0 ;
        gl_FragDepth = depth;

        // lighting calculations (Phonng lighting model)
        // Light pos is hardcoded atm, but should get the information about the lights from scenery and the example itself
        vec3 lightPos = vec3(10.0, 10.0, 10.0);
        vec3 lightColor = vec3(1.0, 0.5, 1.0);
        vec3 lightDir = normalize(lightPos - intersection);

        float ambientStrength = 1.0;
        vec3 ambient = ambientStrength * lightColor;

        float diff = max(dot(lightDir, normal), 0.0);
        vec3 diffuse = diff * lightColor;

        vec3 viewDir = normalize(u_ActiveCamera.Position.xyz - intersection);
        vec3 reflectDir = reflect(-lightDir, normal);
        float specularStrength = 0.2;
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * lightColor;

        vec3 result = (ambient + diffuse + specular) * SilhouetteCorner.Color.rgb;
        FragColor = vec4(result, SilhouetteCorner.Color.a);
    }
}

