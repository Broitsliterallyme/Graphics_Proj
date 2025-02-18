#version 430 core

// Work-group size for image processing.
layout(local_size_x = 16, local_size_y = 16) in;

// Output image.
layout(rgba32f, binding = 0) uniform image2D resultImage;

// Uniform inputs for image resolution, camera position and ray direction.
uniform vec2 iResolution;
uniform vec3 CameraPos;
uniform vec3 RayDir;

// Voxel grid parameters.
const float voxelSize = 0.2;
const ivec3 gridSize = ivec3(100, 100, 100);  // example grid dimensions

// SSBO for voxel grid occupancy data.
// Each element represents one voxel: 0 for empty, nonzero for occupied.
layout(std430, binding = 1) buffer VoxelGridBuffer {
    int voxelData[];
};

// Helper function: convert a world-space position to grid coordinates.
ivec3 worldToGrid(vec3 pos) {
    return ivec3(floor(pos / voxelSize));
}

// Returns true if the voxel at the given coordinate is occupied.
bool isVoxelOccupied(ivec3 voxel) {
    if (voxel.x < 0 || voxel.x >= gridSize.x ||
        voxel.y < 0 || voxel.y >= gridSize.y ||
        voxel.z < 0 || voxel.z >= gridSize.z) {
        return false;
    }
    int index = voxel.x + gridSize.x * (voxel.y + gridSize.y * voxel.z);
    return (voxelData[index] != 0);
}

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(iResolution.x) || pixel.y >= int(iResolution.y)) return;

    // Reconstruct per-pixel UV coordinates.
    vec2 uv = (vec2(pixel) / iResolution) * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;

    // Build camera coordinate system using the provided RayDir.
    vec3 forward = normalize(RayDir);
    vec3 right = normalize(cross(forward, vec3(0.0, 1.0, 0.0)));
    vec3 up = normalize(cross(right, forward));
    float fov = tan(radians(45.0) / 2.0);
    vec3 rd = normalize(forward + uv.x * right * fov + uv.y * up * fov);
    vec3 ro = CameraPos;

    // Initialize voxel traversal.
    ivec3 voxel = worldToGrid(ro);
    ivec3 step = ivec3(sign(rd));

    // Compute the position of the next voxel boundaries.
    vec3 nextBoundary;
    nextBoundary.x = (step.x > 0) ? float(voxel.x + 1) * voxelSize : float(voxel.x) * voxelSize;
    nextBoundary.y = (step.y > 0) ? float(voxel.y + 1) * voxelSize : float(voxel.y) * voxelSize;
    nextBoundary.z = (step.z > 0) ? float(voxel.z + 1) * voxelSize : float(voxel.z) * voxelSize;

    // Compute tMax: the distance along the ray to the next voxel boundary.
    vec3 tMax;
    tMax.x = (rd.x != 0.0) ? (nextBoundary.x - ro.x) / rd.x : 1e20;
    tMax.y = (rd.y != 0.0) ? (nextBoundary.y - ro.y) / rd.y : 1e20;
    tMax.z = (rd.z != 0.0) ? (nextBoundary.z - ro.z) / rd.z : 1e20;

    // Compute tDelta: how far we must move along the ray to cross one voxel.
    vec3 tDelta;
    tDelta.x = (rd.x != 0.0) ? voxelSize / abs(rd.x) : 1e20;
    tDelta.y = (rd.y != 0.0) ? voxelSize / abs(rd.y) : 1e20;
    tDelta.z = (rd.z != 0.0) ? voxelSize / abs(rd.z) : 1e20;

    // Voxel traversal loop: try up to a fixed maximum number of steps.
    bool hit = false;
    vec3 hitColor = vec3(0.0);
    int maxSteps = 500;
    for (int i = 0; i < maxSteps; ++i) {
        if (isVoxelOccupied(voxel)) {
            hit = true;
            // Generate a simple color from voxel coordinates.
            hitColor = vec3(float(voxel.x) / float(gridSize.x),
                            float(voxel.y) / float(gridSize.y),
                            float(voxel.z) / float(gridSize.z));
            break;
        }

        // Step to the next voxel using the smallest tMax.
        if (tMax.x < tMax.y && tMax.x < tMax.z) {
            voxel.x += step.x;
            tMax.x += tDelta.x;
        } else if (tMax.y < tMax.z) {
            voxel.y += step.y;
            tMax.y += tDelta.y;
        } else {
            voxel.z += step.z;
            tMax.z += tDelta.z;
        }
    }

    // Write the output color.
    vec4 color = hit ? vec4(hitColor, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
    imageStore(resultImage, pixel, color);
}
