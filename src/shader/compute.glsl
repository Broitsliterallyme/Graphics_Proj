#version 430 core

// Work-group size for image processing.
layout(local_size_x = 16, local_size_y = 16) in;

// Output image.
layout(rgba32f, binding = 0) uniform image2D resultImage;

// Uniforms for image resolution, camera position and ray direction.
uniform vec2 iResolution;
uniform vec3 CameraPos;
uniform vec3 RayDir;

// Voxel grid parameters.
const float voxelSize = 0.08;
const ivec3 gridSize = ivec3(600, 150, 600);  // 1,000,000 voxels

// SSBO for voxel grid occupancy data.
// Each element represents one voxel: 0 for empty, nonzero for occupied.
layout(std430, binding = 1) buffer VoxelGridBuffer {
    int voxelData[];
};

// Convert world-space position to grid coordinates.
ivec3 worldToGrid(vec3 pos) {
    return ivec3(floor(pos / voxelSize));
}

// Checks if the voxel at coordinate v is occupied.
bool isVoxelOccupied(ivec3 v) {
    if (v.x < 0 || v.x >= gridSize.x ||
        v.y < 0 || v.y >= gridSize.y ||
        v.z < 0 || v.z >= gridSize.z)
        return false;
    int index = v.x + gridSize.x * (v.y + gridSize.y * v.z);
    return voxelData[index] != 0;
}

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(iResolution.x) || pixel.y >= int(iResolution.y))
        return;

    // Compute normalized UV coordinates.
    vec2 uv = (vec2(pixel) / iResolution) * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;
    
    // Build a per-pixel ray using a simple pinhole model.
    vec3 forward = normalize(RayDir);
    vec3 right = normalize(cross(forward, vec3(0.0, 1.0, 0.0)));
    vec3 up = cross(right, forward); // Already normalized if forward/right are normalized.
    float fov = tan(radians(45.0) * 0.5);
    vec3 rd = normalize(forward + uv.x * right * fov + uv.y * up * fov);
    vec3 ro = CameraPos;
    
    // Initialize voxel traversal.
    ivec3 voxel = worldToGrid(ro);
    ivec3 step = ivec3(sign(rd));
    
    // Compute the position of the next voxel boundary for each axis.
    vec3 nextBoundary;
    nextBoundary.x = (step.x > 0) ? (float(voxel.x + 1) * voxelSize) : (float(voxel.x) * voxelSize);
    nextBoundary.y = (step.y > 0) ? (float(voxel.y + 1) * voxelSize) : (float(voxel.y) * voxelSize);
    nextBoundary.z = (step.z > 0) ? (float(voxel.z + 1) * voxelSize) : (float(voxel.z) * voxelSize);
    
    // Calculate tMax: distance along the ray to the next voxel boundary.
    vec3 tMax = vec3(1e20);
    tMax.x = (abs(rd.x) > 0.0) ? (nextBoundary.x - ro.x) / rd.x : 1e20;
    tMax.y = (abs(rd.y) > 0.0) ? (nextBoundary.y - ro.y) / rd.y : 1e20;
    tMax.z = (abs(rd.z) > 0.0) ? (nextBoundary.z - ro.z) / rd.z : 1e20;
    
    // Calculate tDelta: distance required to cross one voxel.
    vec3 tDelta = vec3(1e20);
    tDelta.x = (abs(rd.x) > 0.0) ? voxelSize / abs(rd.x) : 1e20;
    tDelta.y = (abs(rd.y) > 0.0) ? voxelSize / abs(rd.y) : 1e20;
    tDelta.z = (abs(rd.z) > 0.0) ? voxelSize / abs(rd.z) : 1e20;
    
    bool hit = false;
    vec3 hitColor = vec3(0.0);
    const int maxSteps = 1000;
    
    // Traverse the voxel grid.
    for (int i = 0; i < maxSteps; ++i) {
        if (isVoxelOccupied(voxel)) {
            hit = true;
            // Color based on voxel position.
            hitColor = vec3(float(voxel.x) / float(gridSize.x),
                            float(voxel.y) / float(gridSize.y),
                            float(voxel.z) / float(gridSize.z));
            break;
        }
        // Advance to the next voxel: choose the smallest tMax component.
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
    
    // Output the final color.
    vec4 outColor = hit ? vec4(hitColor, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
    imageStore(resultImage, pixel, outColor);
}