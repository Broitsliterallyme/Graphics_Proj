#version 330 core
out vec4 FragColor;

uniform vec2 iResolution;   // Screen resolution
uniform mat4 viewMatrix;    // Camera's LookAt matrix
uniform vec3 cameraPos;     // Camera position
uniform float fov;          // Field of view
uniform vec3 cubeCentre;
uniform mat4 rot;

const int MAX_STEPS = 100;
const float MAX_DIST = 100.0;
const float SURFACE_DIST = 0.001;

float cubeSDF(vec3 p , vec3 cube_size_half) {
    p = p - cubeCentre;
    mat3 rot3 = mat3(rot);
    vec3 rp = rot3*p;
    vec3 d = abs(rp) - cube_size_half;
    return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}

float rayMarch(vec3 ro, vec3 rd) {
    float dO = 0.0;
    for (int i = 0; i < MAX_STEPS; i++) {
        vec3 p = ro + rd * dO;
        float dS = cubeSDF(p , vec3(1));
        dO += dS;
        if (dO > MAX_DIST || dS < SURFACE_DIST) break;
    }
    return dO;
}

void main()
{
    vec2 uv = (gl_FragCoord.xy / iResolution.xy) * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y; // Correct aspect ratio
    vec3 rayDirCameraSpace = normalize(vec3(uv, (-1.0 / tan(radians(fov / 2.0))  )));
    //vec3 rayDirWorldSpace = normalize((viewMatrix * vec4(rayDirCameraSpace, 0.0)).xyz);
    mat3 invViewMatrix = mat3(transpose(viewMatrix)); // Inverse for rotation matrix
    vec3 rayDirWorldSpace = normalize(invViewMatrix * rayDirCameraSpace);

    vec3 rayOrigin = cameraPos;

    float d = rayMarch(rayOrigin, rayDirWorldSpace);

    if (d < MAX_DIST) {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red for the cube
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0); // Black for the background
    }
}
