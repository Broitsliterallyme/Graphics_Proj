#version 430 core
#ifdef GL_ES
precision mediump float; // For ES targets; desktop GL ignores these.
#endif

// Work-group size for image processing.
layout(local_size_x = 32, local_size_y = 32) in;

// Output image.
layout(rgba32f, binding = 0) uniform image2D resultImage;

// Uniforms for image resolution, camera position, and ray direction.
uniform vec2 iResolution;
uniform vec3 CameraPos;
uniform vec3 RayDir;

// Voxel grid parameters.
const float voxelSize = 0.08;
const ivec3 gridSize = ivec3(1000, 150, 1000);
const int gridXY = gridSize.x * gridSize.y;
const float invVoxelSize = 1.0 / voxelSize;
struct Voxel {
    uint occupancy; // 0 = empty, nonzero = occupied.
    float R;
    float G;
    float B;
};
// SSBO for voxel grid occupancy data (0 for empty, nonzero for occupied).
layout(std430, binding = 1) buffer VoxelGridBuffer {
    Voxel voxelData[];
};

// Convert a world-space position to grid coordinates.
ivec3 worldToGrid(vec3 pos) {
    return ivec3(floor(pos * invVoxelSize));
}

// Checks if the voxel at coordinate v is occupied.
bool isVoxelOccupied(int index) {
    return voxelData[index].occupancy != 0;
}

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(iResolution.x) || pixel.y >= int(iResolution.y))
        return;
    
    // Compute normalized UV coordinates.
    vec2 uv = (vec2(pixel) / iResolution) * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;
    
    // Build per-pixel ray using a simple pinhole model.
    vec3 forward = normalize(RayDir);
    vec3 right = normalize(cross(forward, vec3(0.0, 1.0, 0.0)));
    vec3 up = cross(right, forward);
    const float halfFov = tan(radians(22.5)); // Half of 45° FOV.
    vec3 rd = normalize(forward + uv.x * right * halfFov + uv.y * up * halfFov);
    vec3 ro = CameraPos;
    
    // Compute grid axis-aligned bounding box.
    vec3 gridMin = vec3(0.0);
    vec3 gridMax = vec3(gridSize) * voxelSize;
    
    // Compute intersection of the ray with the grid's AABB using the slab method.
    vec3 invRd = vec3(1.0) / rd;
    vec3 t0 = (gridMin - ro) * invRd;
    vec3 t1 = (gridMax - ro) * invRd;
    vec3 tMinVec = min(t0, t1);
    vec3 tMaxVec = max(t0, t1);
    float tEntry = max(max(tMinVec.x, tMinVec.y), tMinVec.z);
    float tExit = min(min(tMaxVec.x, tMaxVec.y), tMaxVec.z);
    
    // If there is no intersection, output the background color.
    if (tExit < 0.0 || tEntry > tExit) {
        imageStore(resultImage, pixel, vec4(0.678, 0.847, 0.902, 1.0));
        return;
    }
    
    // Start the ray at the grid entry point (or at the camera if already inside).
    float t = max(tEntry, 0.0);
    ro += t * rd;
    const float epsilon = 0.0001; // Small offset to avoid precision issues at boundaries.
    ro += rd * epsilon;
    
    ivec3 voxel = worldToGrid(ro);
    
    // Compute the next voxel boundary positions.
    vec3 nextBoundary;
    nextBoundary.x = (rd.x >= 0.0) ? (float(voxel.x + 1) * voxelSize) : (float(voxel.x) * voxelSize);
    nextBoundary.y = (rd.y >= 0.0) ? (float(voxel.y + 1) * voxelSize) : (float(voxel.y) * voxelSize);
    nextBoundary.z = (rd.z >= 0.0) ? (float(voxel.z + 1) * voxelSize) : (float(voxel.z) * voxelSize);
    
    // Precompute distances to the next boundaries (tMax) and per-voxel step distances (tDelta).
    vec3 tMax = vec3(1e20);
    tMax.x = (abs(rd.x) > 0.0) ? (nextBoundary.x - ro.x) * invRd.x : 1e20;
    tMax.y = (abs(rd.y) > 0.0) ? (nextBoundary.y - ro.y) * invRd.y : 1e20;
    tMax.z = (abs(rd.z) > 0.0) ? (nextBoundary.z - ro.z) * invRd.z : 1e20;
    
    vec3 tDelta;
    tDelta.x = (abs(rd.x) > 0.0) ? voxelSize * abs(invRd.x) : 1e20;
    tDelta.y = (abs(rd.y) > 0.0) ? voxelSize * abs(invRd.y) : 1e20;
    tDelta.z = (abs(rd.z) > 0.0) ? voxelSize * abs(invRd.z) : 1e20;
    
    bool hit = false;
    vec3 hitColor = vec3(0.0);
    const int maxSteps = 700;
    
    // Traverse the voxel grid.
    for (int i = 0; i < maxSteps; ++i) {
        // If voxel is out-of-bounds, stop traversal.
        if (voxel.x < 0 || voxel.x >= gridSize.x ||
            voxel.y < 0 || voxel.y >= gridSize.y ||
            voxel.z < 0 || voxel.z >= gridSize.z)
        {
            break;
        }
        // If the nearest voxel boundary is beyond the grid exit, stop.
        if (min(tMax.x, min(tMax.y, tMax.z)) > tExit)
            break;
        int index = voxel.x + gridSize.x * voxel.y + gridXY * voxel.z;

        if (isVoxelOccupied(index)) {
            hit = true;
            // Color the hit voxel based on its grid coordinates.
            hitColor = vec3(voxelData[index].R,voxelData[index].G,voxelData[index].B);
            break;
        }
        
        // Branchless selection of the axis to step:
        int axis = (tMax.x < tMax.y) ? ((tMax.x < tMax.z) ? 0 : 2)
                                     : ((tMax.y < tMax.z) ? 1 : 2);
        
        // Update the voxel coordinate along the chosen axis.
        voxel[axis] += (rd[axis] >= 0.0) ? 1 : -1;
        tMax[axis] += tDelta[axis];
    }
    
    // Output the color: hit color if an occupied voxel was found, else the background.
    vec4 outColor = hit ? vec4(hitColor, 1.0) : vec4(0.678/2, 0.847/2, 0.902/2, 1.0);
    imageStore(resultImage, pixel, outColor);
}
