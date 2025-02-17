#version 430 core

layout (local_size_x = 16, local_size_y = 16) in;
layout (rgba32f, binding = 0) uniform image2D resultImage;
uniform vec2 iResolution;  // Image resolution (width, height)
uniform float iTime;       // Time for animation (optional)
uniform vec3 cameraPosition;  // Camera position in world space
uniform vec3 cameraTarget;     // Where the camera is looking at
uniform float fov;             // Field of view

float sphere(vec3 p, float radius) {
    return length(p) - radius;
}

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(iResolution.x) || pixel.y >= int(iResolution.y)) return;

    vec2 uv = vec2(pixel) / iResolution;
    uv.x *= iResolution.x / iResolution.y;  // Adjust for aspect ratio

    // Calculate ray direction from camera
    vec3 forward = normalize(cameraTarget - cameraPosition);  // Direction the camera is facing
    vec3 right = normalize(cross(forward, vec3(0.0, 1.0, 0.0)));  // Right vector (cross product with up)
    vec3 up = cross(right, forward);  // Up vector (cross product with right)

    // Use UV coordinates to create a direction from the camera view
    float aspectRatio = iResolution.x / iResolution.y;
    float tanFov = tan(radians(fov) * 0.5);  // Half field of view tangent

    // Adjust the ray direction based on the aspect ratio and FOV
    vec3 rd = normalize(uv.x * tanFov * right + uv.y * tanFov * up + forward);

    // Raymarching parameters
    vec3 ro = cameraPosition;  // Camera position
    vec3 sphereCenter = vec3(0.0, 0.0, 0.0);  // Sphere center at origin
    float sphereRadius = 0.5;

    // Raymarching loop (fixed steps)
    float t = 0.0;
    const int maxSteps = 100;
    const float maxDistance = 100.0;
    float distance = sphere(ro + rd * t, sphereRadius);
    for (int i = 0; i < maxSteps && distance > 0.001 && t < maxDistance; i++) {
        t += distance;
        distance = sphere(ro + rd * t, sphereRadius);
    }

    // Color the scene
    vec4 color;
    if (distance < 0.001) {
        color = vec4(1.0, 0.0, 0.0, 1.0);  // Red for the sphere
    } else {
        color = vec4(0.0, 0.0, 0.0, 1.0);  // Black background
    }

    // Store the result in the image
    imageStore(resultImage, pixel, color);
}
