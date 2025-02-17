#version 430 core

layout (local_size_x = 16, local_size_y = 16) in;
layout (rgba32f, binding = 0) uniform image2D resultImage;

uniform vec2 iResolution; // Screen resolution
uniform vec3 iCenter;     // Cube center in normalized coordinates (0 to 1)
float iSize=0.1;      // Cube size in normalized coordinates (0 to 1)

float box(vec3 p, vec3 s) {
    // Returns the minimum distance to a box from a point p
    vec3 d = abs(p) - s;
    return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(iResolution.x) || pixel.y >= int(iResolution.y)) return;

    // Normalize pixel coordinates
    vec2 uv = vec2(pixel) / iResolution;

    // Adjust for aspect ratio
    uv.x *= iResolution.x / iResolution.y;

    // Map to normalized device coordinates (-1, 1) space
    vec3 ro = vec3(0.0, 0.0, -2.0); // Camera position
    vec3 rd = normalize(vec3(uv - 0.5, 1.0)); // Ray direction

    // Cube center and size in world space
    vec3 cubeCenter = (iCenter * 2.0) - 1.0; // Convert to [-1,1] space
    vec3 cubeSize = vec3(iSize); // Cube size

    // Raymarching to find cube surface
    float t = 0.0;
    const int maxSteps = 100;
    const float maxDistance = 100.0;
    float distance = box(ro + rd * t - cubeCenter, cubeSize); // Distance to cube
    for (int i = 0; i < maxSteps && distance > 0.001 && t < maxDistance; i++) {
        t += distance;  // Move along the ray
        distance = box(ro + rd * t - cubeCenter, cubeSize);  // Recalculate distance
    }

    // Output color
    vec4 color;
    if (distance < 0.001) {
        color = vec4(1.0, 0.0, 0.0, 1.0); // Red for the cube
    } else {
        color = vec4(0.0, 0.0, 0.0, 1.0); // Black background
    }

    imageStore(resultImage, pixel, color);  // Store color at the pixel location
}
