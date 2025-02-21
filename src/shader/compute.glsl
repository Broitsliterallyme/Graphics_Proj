#version 430 core

layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) uniform image2D resultImage;

struct FlattenedNode {
    bool IsLeaf;
    int childIndices[8];
    vec4 color;
};

layout(std430, binding = 1) buffer NodeBuffer {
    FlattenedNode nodes[];
};

uniform vec2 iResolution;
uniform mat4 viewMatrix;
uniform vec3 cameraPos;
uniform float fov;
uniform vec3 minBound;
uniform vec3 maxBound;
float lodThreshold=0.015;  // Controls when to stop subdividing based on projected size

const float MAX_DIST = 4000.0;
const int MAX_STACK_SIZE = 16;

// Stack entry structure for iterative traversal.
struct StackEntry {
    int nodeIndex;
    vec3 nodeMin;
    vec3 nodeMax;
    float tEnter;
};

// Improved AABB intersection that uses the precomputed inverse ray direction.
bool intersectAABB(vec3 ro, vec3 rd, vec3 invRD, vec3 boxMin, vec3 boxMax, out float tEnter, out float tExit) {
    vec3 t1 = (boxMin - ro) * invRD;
    vec3 t2 = (boxMax - ro) * invRD;
    vec3 tmin = min(t1, t2);
    vec3 tmax = max(t1, t2);
    tEnter = max(max(tmin.x, tmin.y), tmin.z);
    tExit  = min(min(tmax.x, tmax.y), tmax.z);
    return (tEnter <= tExit && tExit > 0.0);
}

// Helper function to compute a child's AABB from its parent's bounds.
void computeChildAABB(int child, vec3 parentMin, vec3 parentMax, out vec3 childMin, out vec3 childMax) {
    vec3 center = (parentMin + parentMax) * 0.5;
    childMin.x = ((child & 4) == 0) ? parentMin.x : center.x;
    childMax.x = ((child & 4) == 0) ? center.x    : parentMax.x;
    childMin.y = ((child & 2) == 0) ? parentMin.y : center.y;
    childMax.y = ((child & 2) == 0) ? center.y    : parentMax.y;
    childMin.z = ((child & 1) == 0) ? parentMin.z : center.z;
    childMax.z = ((child & 1) == 0) ? center.z    : parentMax.z;
}

vec4 traverseOctree(vec3 ro, vec3 rd) {
    float tEnterRoot, tExitRoot;
    vec3 invRD = vec3(1.0) / rd; // Precompute reciprocal of the ray direction.
    if (!intersectAABB(ro, rd, invRD, minBound, maxBound, tEnterRoot, tExitRoot)) {
        return vec4(0.0);
    }
    
    // Initialize a fixed-size stack for iterative traversal.
    StackEntry stack[MAX_STACK_SIZE];
    int stackSize = 0;
    stack[stackSize++] = StackEntry(0, minBound, maxBound, tEnterRoot);
    
    float bestT = MAX_DIST;
    vec4 hitColor = vec4(0.0);
    
    while (stackSize > 0) {
        // Find the stack entry with the smallest tEnter.
        int bestIndex = 0;
        float currentBest = stack[0].tEnter;
        for (int i = 1; i < stackSize; i++) {
            if (stack[i].tEnter < currentBest) {
                currentBest = stack[i].tEnter;
                bestIndex = i;
            }
        }
        
        StackEntry entry = stack[bestIndex];
        stack[bestIndex] = stack[stackSize - 1];
        stackSize--;
        
        if (entry.tEnter > bestT)
            continue;
        
        FlattenedNode node = nodes[entry.nodeIndex];

        // Compute the node's center, size, and its distance from the camera.
        vec3 nodeCenter = (entry.nodeMin + entry.nodeMax) * 0.5;
        float distance = length(cameraPos - nodeCenter);
        float nodeSize = length(entry.nodeMax - entry.nodeMin);
        // Compute a simple LOD metric (larger ratio means the node is larger on screen).
        // When the ratio is below the threshold, we consider this node detailed enough.
        float lodMetric = nodeSize / max(distance, 0.001);
        
        // If the node is a leaf, or if the LOD metric is low enough, treat this node as a final hit.
        if (node.IsLeaf || (!node.IsLeaf && lodMetric < lodThreshold)) {
            float hitT = entry.tEnter;
            vec3 hitPoint = ro + hitT * rd;
            
            // Compute a simple normal based on which face of the AABB is hit.
            float distXMin = abs(hitPoint.x - entry.nodeMin.x);
            float distXMax = abs(entry.nodeMax.x - hitPoint.x);
            float distYMin = abs(hitPoint.y - entry.nodeMin.y);
            float distYMax = abs(entry.nodeMax.y - hitPoint.y);
            float distZMin = abs(hitPoint.z - entry.nodeMin.z);
            float distZMax = abs(entry.nodeMax.z - hitPoint.z);
            
            vec3 normal = vec3(-1.0, 0.0, 0.0);
            float minDist = distXMin;
            if (distXMax < minDist) { minDist = distXMax; normal = vec3(1.0, 0.0, 0.0); }
            if (distYMin < minDist) { minDist = distYMin; normal = vec3(0.0, -1.0, 0.0); }
            if (distYMax < minDist) { minDist = distYMax; normal = vec3(0.0, 1.0, 0.0); }
            if (distZMin < minDist) { minDist = distZMin; normal = vec3(0.0, 0.0, -1.0); }
            if (distZMax < minDist) { minDist = distZMax; normal = vec3(0.0, 0.0, 1.0); }
            
            const vec3 sunDir = normalize(vec3(0.4, 1.0, -1.0));
            float diffuse = max(dot(normal, sunDir), 0.0);
            float ambient = 0.3;
            float lighting = clamp(ambient + 0.7 * diffuse, 0.0, 1.0);
            
            hitColor = vec4(node.color.rgb * lighting, 1.0);
            bestT = entry.tEnter;
            break;
        }
        
        // Otherwise, subdivide and traverse children.
        for (int child = 0; child < 8; child++) {
            int childIndex = node.childIndices[child];
            if (childIndex == -1)
                continue;
            
            vec3 childMin, childMax;
            computeChildAABB(child, entry.nodeMin, entry.nodeMax, childMin, childMax);
            
            float tChildEnter, tChildExit;
            if (intersectAABB(ro, rd, invRD, childMin, childMax, tChildEnter, tChildExit)) {
                if (tChildEnter < bestT && stackSize < MAX_STACK_SIZE)
                    stack[stackSize++] = StackEntry(childIndex, childMin, childMax, tChildEnter);
            }
        }
    }
    
    return vec4(hitColor.xyz, 1.0);
}

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    if (pixelCoords.x >= int(iResolution.x) || pixelCoords.y >= int(iResolution.y))
        return;
    
    vec2 uv = (vec2(pixelCoords) / iResolution) * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;
    // Compute the ray direction in camera space using the field of view.
    vec3 rayDirCamera = normalize(vec3(uv, -1.0 / tan(radians(fov * 0.5))));
    mat3 invViewMatrix = mat3(transpose(viewMatrix));
    vec3 rayDirWorld = normalize(invViewMatrix * rayDirCamera);
    
    vec4 color = traverseOctree(cameraPos, rayDirWorld);
    imageStore(resultImage, pixelCoords, color);
}
