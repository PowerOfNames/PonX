#type vertex
#version 430 core	
layout(location = 0) in vec3 a_Position;		

//uniform mat4 u_ViewProjection;

void main()
{
	gl_Position = vec4(a_Position, 1.0f);
}

#type fragment
#version 430 core		
in vec4 gl_FragCoord;
//layout(location = 0) out vec4 gl_FragColor;

// Camera
uniform vec3 u_CameraPos;
uniform vec3 u_CameraFront;
uniform vec3 u_CameraUp;
uniform vec2 u_WindowDims;

uniform float u_AspectRatio;
uniform int u_FOV;

// Lightsources
uniform vec3 u_PointLightPos;
uniform vec3 u_PointLightColor;
uniform float u_PointLightIntensity;

//uniform mat4 u_ViewProjection;

//Mapdata
uniform sampler2D u_MapData;

//Constants
#define PI 3.14159

//p is the point, c is the sphere centre, r the radius
float sdSphere(in vec3 p, in vec3 centre, float radius)
{
    return length(p - centre) - radius;
}

float sdBox(in vec3 p, in vec3 size, in vec3 position )
{
    vec3 q = abs(p - position) - size;
    return length(max(q, 0.0)) + min(max(q.x,max(q.y, q.z)), 0.0);
}

float map_the_world(in vec3 p)
{
    float cube_0 = sdBox(p, vec3(1.0, 1.0, 1.0), vec3(2.0, 2.0, 2.0));
    float light_0 = sdBox(p, vec3(1.0, 1.0, 1.0), u_PointLightPos + 1.0);
    float sphere_0 = sdSphere(p, vec3(4.0, 1.0, 4.0), 2.0);

    return min(sphere_0, min(cube_0, light_0));
}


vec3 calculate_normal(in vec3 position)
{
    const vec3 small_step = vec3(0.001, 0.0, 0.0);


    float gradient_x = map_the_world(position + small_step.xyy) - map_the_world(position - small_step.xyy);
    float gradient_y = map_the_world(position + small_step.yxy) - map_the_world(position - small_step.yxy);
    float gradient_z = map_the_world(position + small_step.yyx) - map_the_world(position - small_step.yyx);

    vec3 normal = vec3(gradient_x, gradient_y, gradient_z);

    return normalize(normal);
}


bool CheckMap(in int px, in int py)
{
    vec4 blockstate = texture(u_MapData, vec2(px, py));
    if(blockstate == vec4(0.0, 0.0, 0.0, 0.0))
    {
        return false;
    }
    else return true;
}

vec3 ray_march(in vec3 origin, in vec3 dir)
{
    float total_distance_traveled = 0.0;
    const int NUMBER_OF_STEPS = 128;
    const float MINIMUM_HIT_DISTANCE = 0.001;
    const float MAXIMUM_TRACE_DISTANCE = 300.0;

    for (int i = 0; i < NUMBER_OF_STEPS; ++i)
    {
        // Calculate our current position along the ray
        vec3 currentPosition = origin + total_distance_traveled * dir;

        // We wrote this function earlier in the tutorial -
        // assume that the sphere is centered at the origin
        // and has unit radius
        float distance_to_closest = map_the_world(currentPosition);


        if (distance_to_closest < MINIMUM_HIT_DISTANCE) // hit
        {
            // We hit something! Return red for now
            vec3 normal = calculate_normal(currentPosition);

            // Calculate the unit direction vector that points from 
            // the point of intersection to the light-source
            vec3 direction_to_light = normalize(currentPosition - u_PointLightPos);
            vec3 viewVector = dir;
        
        //Ambient
            float ambientIntensity = 0.4;
            vec3 ambientColor = vec3(1.0, 1.0, 1.0);
            vec3 ambientLight = ambientColor * ambientIntensity;
        //Diffuse
            //float diffuseIntensity = 0.1;
            //vec3 diffuseColor = vec3(1.0, 1.0, 1.0);
            vec3 diffuseLight = u_PointLightColor * dot(normal, -direction_to_light) * u_PointLightIntensity;
        //Specular
            float roughness = 2.0;
            float specularIntensity = 0.8;
            vec3 reflectionVector = 2.0 * dot(normal, direction_to_light) * normal - direction_to_light;
            float specularLight = specularIntensity * (2*roughness)/(2*PI) * pow(max(0.0, dot(viewVector, reflectionVector)), roughness);

            vec3 color = vec3(32.0/255.0, 95.0/255.0, 83.0/255.0);

            return (ambientLight + diffuseLight + specularLight) * color;
        }

        if (total_distance_traveled > MAXIMUM_TRACE_DISTANCE) // miss
        {
            break;
        }

        // accumulate the distance traveled thus far
        total_distance_traveled += distance_to_closest;
    }

    // If we get here, we didn't hit anything so just
    // return a background color (black)
    return vec3(0.2);
}

//Calculates the direction of the current ray from the cameraPosition through the current pixel
vec3 GetRayWindowIntersection(vec2 uv)
{
    vec2 screenspace_Uv = vec2((2 * (gl_FragCoord.x +uv.x)/u_WindowDims.x - 1.0) * u_AspectRatio * tan(u_FOV/2 * PI / 180), (1 - 2.0 * (gl_FragCoord.y +uv.y)/u_WindowDims.y) * tan(u_FOV/2 * PI / 180));
    
    vec3 cameraRight = cross(u_CameraFront, u_CameraUp);
    vec3 cameraUp = cross(u_CameraFront, cameraRight);
    vec3 direction = cameraRight * screenspace_Uv.x + cameraUp * screenspace_Uv.y + u_CameraPos + u_CameraFront;
    
    return direction;
}


void main()
{
    
    vec3 color = vec3(0.0);
    int resolution_Factor = 4; // -> 4*4 rays per pixel
    for(int i = 0; i < resolution_Factor; i++)
    {
        for(int j = 0; j < resolution_Factor; j++)
        {
            color += ray_march(u_CameraPos, normalize(GetRayWindowIntersection(vec2(i * 0.25, j * 0.25)) - u_CameraPos));
        }
    }
    color /= resolution_Factor * resolution_Factor;
    float gamma = 1.5;
    color = pow(color, vec3(1.0/gamma));
    gl_FragColor = vec4(color, 0.6);
}
