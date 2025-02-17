#version 430 core
layout (local_size_x = 16, local_size_y = 16) in;
layout (rgba32f, binding = 0) uniform image2D resultImage;
uniform vec2 iResolution;

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(iResolution.x) || pixel.y >= int(iResolution.y)) return;

    vec2 uv = vec2(pixel) / iResolution;
    vec4 color = vec4(uv, 0.9, 1.0); // Gradient from black to blue

    imageStore(resultImage, pixel, color);
}
