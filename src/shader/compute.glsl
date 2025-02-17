#version 430 core

layout (local_size_x = 16, local_size_y = 16) in;
layout (rgba32f, binding = 0) uniform image2D resultImage;

uniform vec2 iResolution; // Screen resolution
uniform vec2 iCenter;     // Circle center in screen space (0 to 1)
float iRadius=0.1;    // Circle radius (0 to 1)

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(iResolution.x) || pixel.y >= int(iResolution.y)) return;

    // Normalize pixel coordinates
    vec2 uv = vec2(pixel) / iResolution;

    // Adjust for aspect ratio
    uv.x *= iResolution.x / iResolution.y;

    // Distance from circle center
    float dist = length(uv - iCenter);
    float alpha = smoothstep(iRadius, iRadius - 0.01, dist); // Smooth edge

    // White circle on black background
    vec4 color = mix(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), alpha);
    imageStore(resultImage, pixel, color);
}
