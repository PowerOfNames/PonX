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
#define MAP_DEPTH 2
#define MAP_HEIGHT 1

float sdBox(in vec3 p, in vec3 size, in vec3 position )
{
    vec3 q = abs(p - position) - size;
    return length(max(q, 0.0)) + min(max(q.x,max(q.y, q.z)), 0.0);
}

vec3 GridRayMarch(in vec3 camPos, in vec3 dir)
{
    vec3 gridOrigin = vec3(0.0, 0.0, 0.0);              // the bottom front most left corner of the given grid
    vec3 rayPos = camPos;
    ivec3 cellIndex = ivec3(camPos);
    ivec3 cellIndexIt;
    vec3 deltaT;                                        // what is missing to the next cellborder in x, y, z, respectivly
    vec3 tMax;                                          // current t to the next border

    if (dir.x < 0)
    {
        deltaT.x = -1/dir.x;
        cellIndexIt.x = -1;
        tMax.x = (cellIndex.x - rayPos.x) / dir.x;
    }
    else if (dir.x > 0)
    {
        deltaT.x = 1/dir.x;
        cellIndexIt.x = 1;
        tMax.x = (cellIndex.x + 1.0 - rayPos.x) / dir.x;
    }
    else
    {
        deltaT.x = 1000;
        cellIndexIt.x = 0;
        tMax.x = 0.0;
    }


    if (dir.y < 0)
    {
        deltaT.y = -1/dir.y;
        cellIndexIt.y = -1;
        tMax.y = (cellIndex.y - rayPos.y) / dir.y;
    }
    else if (dir.y > 0)
    {
        deltaT.y = 1/dir.y;
        cellIndexIt.y = 1;
        tMax.y = (cellIndex.y + 1.0 - rayPos.y) / dir.y;
    }
    else
    {
        deltaT.y = 1000;
        cellIndexIt.y = 0;
        tMax.y = 0.0;
    }

    if (dir.z < 0)
    {
        deltaT.z = -1/dir.z;
        cellIndexIt.z = -1;
        tMax.z = (cellIndex.z - rayPos.z) / dir.z;
    }
    else if (dir.z > 0)
    {
        deltaT.z = 1/dir.z;
        cellIndexIt.z = 1;
        tMax.z = (cellIndex.z + 1.0 - rayPos.z) / dir.z;
    }
    else
    {
        deltaT.z = 1000;
        cellIndexIt.z = 0;
        tMax.z = 0.0;
    }

    vec3 backgroundX = vec3(1.0, 0.0, 0.0);
    vec3 backgroundY = vec3(0.0, 1.0, 0.0);
    vec3 backgroundZ = vec3(0.0, 0.0, 1.0);
    float t = 0;                                        // mutliplier for the direction 
    vec4 voxel;
    while(t < 200)
    {
        if (tMax.x < tMax.y)
        {
            if (tMax.x < tMax.z)
            {
                t = tMax.x;
                tMax.x += deltaT.x;
                if (dir.x < 0)
                    cellIndex.x -= 1;
                    if(cellIndex.x < 0)
                        return backgroundX;
                else
                    cellIndex.x += 1;
                    if(cellIndex.x > MAP_WIDTH -1)
                        return backgroundX;
            }
            else
            {
                t = tMax.z;
                tMax.z += deltaT.z;
                if (dir.z < 0)
                    cellIndex.z -= 1;
                    if(cellIndex.z < 0)
                        return backgroundZ;
                else
                    cellIndex.z += 1;
                    if(cellIndex.z > MAP_DEPTH -1)
                        return backgroundZ;
            }
        } // end if
        else
        {
            if (tMax.y < tMax.z)
            {
                t = tMax.y;
                tMax.y += deltaT.y;
                if (dir.y < 0)
                    cellIndex.y -= 1;
                    if(cellIndex.y < 0)
                        return backgroundY;
                else
                    cellIndex.y += 1;
                    if(cellIndex.y > MAP_HEIGHT -1)
                        return backgroundY;
            }
            else
            {
                t = tMax.z;
                tMax.z += deltaT.z;
                if (dir.z < 0)
                    cellIndex.z -= 1;
                    if(cellIndex.z < 0)
                        return backgroundZ;
                else
                    cellIndex.z += 1;
                    if(cellIndex.z > MAP_DEPTH -1)
                        return backgroundZ;
            }
        } // end else

        // This if statement is not final, because at the moment it will always break form the loop if the rtay is outside of the grid, which I do not want
        // The goal is to tell when the ray got through the whole grtid without finding something and then terminate.
        // At the moment, is terminates if the ray is in front of the grid and didnt even reach it.

        // To fix this, one possible solution would be to check before the loop if the current ray would ever hit the grid. This, however, would just be a temporary solution, cause at the end of the day,
        //the grid should be everywhere (In this particular case of ray-tracing accelaration-structure)
        // In addition to that, I would have to to a normal ray-box intersection test for every ray several times to decide on hitting or not. That would be redundant, 
        //because then I could direcly render a cube at the given positions

        // So instead, I should use the current ray position, check if it is within the grid and do the mapData-retrieving AFTER that check is true.
        // This will result in a couple of If- statements, which may not be the most efficient way to do, but on that problem later
        if(cellIndex.x > -1 && cellIndex.x < MAP_WIDTH && cellIndex.y > -1 && cellIndex.y < MAP_HEIGHT && cellIndex.z > -1 && cellIndex.z < MAP_DEPTH)
            voxel = texture(u_MapData, vec2( cellIndex.x, cellIndex.y * MAP_DEPTH * MAP_WIDTH + cellIndex.z * MAP_WIDTH));

        if(voxel.a != 0x00)
            return voxel.xyz;

    } // end while

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
            vec3 dir = normalize(GetRayWindowIntersection(vec2(i * 0.25, j * 0.25)) - u_CameraPos);
            color += GridRayMarch(u_CameraPos, dir);
        }
    }
    color /= resolution_Factor * resolution_Factor;
    float gamma = 1.5;
    color = pow(color, vec3(1.0/gamma));
    gl_FragColor = vec4(color, 1.0);
}
