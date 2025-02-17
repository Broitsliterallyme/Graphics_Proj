#version 430 core

layout (local_size_x = 16, local_size_y = 16) in;
layout (rgba32f, binding = 0) uniform image2D resultImage;
uniform vec2 iResolution; // Image resolution (width, height)
uniform float iTime;      // Time for animation (optional, can be used for rotation)

float sphere(vec3 p, float radius) {
    // Returns the distance from point `p` to the surface of a sphere with radius `radius`
    return length(p) - radius;
}

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(iResolution.x) || pixel.y >= int(iResolution.y)) return;

    // Normalized pixel coordinates (0 to 1)
    vec2 uv = vec2(pixel) / iResolution;

    // Adjust for aspect ratio
    uv.x *= iResolution.x / iResolution.y;

    // Convert 2D pixel coordinates to 3D space
    // Use uv.x and uv.y to create the x and y coordinates, and add a "z" based on the time
    vec3 ro = vec3(0.0, 0.0, -3.0);  // Camera position (origin point)
    vec3 rd = normalize(vec3(uv - 0.5, 1.0)); // Direction of the ray, looking at (0, 0, 0)

    // Sphere properties
    vec3 sphereCenter = vec3(0.0, 0.0, 0.0);  // Sphere center at the origin
    float sphereRadius = 0.5;  // Sphere radius

    // Raymarching loop (simple implementation, fixed maximum steps)
    float t = 0.0;
    const int maxSteps = 100;  // Maximum number of raymarching steps
    const float maxDistance = 100.0;  // Maximum distance we want to march before giving up
    float distance = sphere(ro + rd * t, sphereRadius);  // Initial distance to the sphere
    for (int i = 0; i < maxSteps && distance > 0.001 && t < maxDistance; i++) {
        t += distance;  // Move along the ray
        distance = sphere(ro + rd * t, sphereRadius);  // Recalculate the distance at the new point
    }

    // Determine if we're inside or outside the sphere
    vec4 color;
    if (distance < 0.001) {
        // We hit the sphere, render it with a color
        color = vec4(1.0, 0.0, 0.0, 1.0);  // Red for the sphere
    } else {
        // We didn't hit the sphere, render background color
        color = vec4(0.0, 0.0, 0.0, 1.0);  // Black for the background
    }

    imageStore(resultImage, pixel, color);  // Store the color at the pixel location
}
