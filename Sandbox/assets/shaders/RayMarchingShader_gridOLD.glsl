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

//uniform mat4 u_ViewProjection;

//Mapdata
uniform sampler2D u_MapData;

//Constants
#define PI 3.14159

#define MAP_WIDTH 2
#define MAP_HEIGHT 1
#define MAP_DEPTH 1

float sdBox(in vec3 p, in vec3 size, in vec3 position )
{
    vec3 q = abs(p - position) - size;
    return length(max(q, 0.0)) + min(max(q.x,max(q.y, q.z)), 0.0);
}

vec3 GridRayMarch(in vec3 origin, in vec3 dir)
{
    ivec3 pos = ivec3(origin);
    ivec3 stepAmount;
    vec3 tDelta = abs(1.0 / dir);
    vec3 tMax;
    vec4 voxel;
    int side;

    if (dir.x < 0)
    {
        stepAmount.x = -1;
        tMax.x = (origin.x - pos.x) * tDelta.x;
    }
    else if (dir.x > 0)
    {
        stepAmount.x = 1;
        tMax.x = (pos.x + 1.0 - origin.x) * tDelta.x;
    }
    else
    {
        stepAmount.x = 0;
        tMax.x = 0;
    }

    if (dir.y < 0)
    {
        stepAmount.y = -1;
        tMax.y = (origin.y - pos.y) * tDelta.y;
    }
    else if (dir.y > 0)
    {
        stepAmount.y = 1;
        tMax.y = (pos.y + 1.0 - origin.y) * tDelta.y;
    }
    else
    {
        stepAmount.y = 0;
        tMax.y = 0;
    }

    if (dir.z < 0)
    {
        stepAmount.z = -1;
        tMax.z = (origin.z - pos.z) * tDelta.z;
    }
    else if (dir.z > 0)
    {
        stepAmount.z = 1;
        tMax.z = (pos.z + 1.0 - origin.z) * tDelta.z;
    }
    else
    {
        stepAmount.z = 0;
        tMax.z = 0;
    }

    do
    {
        if (tMax.x < tMax.y)
        {
            if (tMax.x < tMax.z)
            {
                pos.x += stepAmount.x;
                if (pos.x >= MAP_WIDTH && stepAmount.x >= 0 || pos.x < 0 && stepAmount.x <= 0)
                    return vec3(0.2);
                tMax.x += tDelta.x;
                side = 0;
            }
            else
            {
                pos.z += stepAmount.z;
                if (pos.z >= MAP_DEPTH && stepAmount.z >= 0 || pos.z < 0 && stepAmount.z <= 0)
                    return vec3(0.5);
                tMax.z += tDelta.z;
                side = 1;
            }
        }
        else
        {
            if (tMax.y < tMax.z)
            {
                pos.y += stepAmount.y;
                if (pos.y >= MAP_HEIGHT && stepAmount.y >= 0 || pos.y < 0 && stepAmount.y <= 0)
                    return vec3(0.8);
                tMax.y += tDelta.y;
                side = 2;
            }
            else
            {
                pos.z += stepAmount.z;
                if (pos.z >= MAP_DEPTH && stepAmount.z >= 0 || pos.z < 0 && stepAmount.z <= 0)
                    return vec3(0.5);
                tMax.z += tDelta.z;
                side = 1;
            }
        }

        if(((pos.x >= 0) && (pos.x < MAP_WIDTH)) && ((pos.z >= 0) && (pos.z < MAP_DEPTH)) && ((pos.y >= 0) && (pos.y < MAP_HEIGHT)))
            voxel = texture(u_MapData, vec2( pos.x, pos.y * MAP_DEPTH * MAP_WIDTH + pos.z * MAP_WIDTH) );


    } while (voxel.a == 0x00);

    return voxel.xyz;
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
            color += GridRayMarch(u_CameraPos, normalize(GetRayWindowIntersection(vec2(i * 0.25, j * 0.25)) - u_CameraPos));
        }
    }
    color /= resolution_Factor * resolution_Factor;
    float gamma = 1.5;
    color = pow(color, vec3(1.0/gamma));
    gl_FragColor = vec4(color, 0.6);
}