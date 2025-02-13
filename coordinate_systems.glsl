#version 330 core
out vec4 FragColor;

uniform vec2 iResolution;   // Screen resolution
uniform mat4 viewMatrix;    // Camera's LookAt matrix
uniform vec3 cameraPos;     // Camera position
uniform float fov;         // Field of view
uniform vec3 cubeCentre;
const int MAX_STEPS = 100;
const float MAX_DIST = 100.0;
const float SURFACE_DIST = 0.001;

float cubeSDF(vec3 p) {
    vec3 d = (abs(p)-cubeCentre) - vec3(1.0);
    return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

float rayMarch(vec3 ro, vec3 rd) {
    float dO = 0.0;
    for (int i = 0; i < MAX_STEPS; i++) {
        vec3 p = ro + rd * dO;
        float dS = cubeSDF(p);
        dO += dS;
        if (dO > MAX_DIST || dS < SURFACE_DIST) break;
    }
    return dO;
}

void main()
{
    // Screen space coordinates to NDC
    vec2 uv = (gl_FragCoord.xy / iResolution.xy) * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y; // Correct aspect ratio

    // Ray direction in camera space
    vec3 rayDirCameraSpace = normalize(vec3(uv, -1.0 / tan(radians(fov / 2.0))));
    
    // Transform ray direction to world space using the LookAt matrix
    vec3 rayDirWorldSpace = normalize((viewMatrix * vec4(rayDirCameraSpace, 0.0)).xyz);

    // Ray origin is the camera's world position
    vec3 rayOrigin = cameraPos;

    // Perform ray marching
    float d = rayMarch(rayOrigin, rayDirWorldSpace);

    // Color the fragment based on distance to the cube
    if (d < MAX_DIST) {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red for the cube
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0); // Black for the background
    }
}
