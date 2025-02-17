#version 430 core
out vec4 FragColor;

uniform vec2 iResolution;
uniform mat4 viewMatrix;
uniform vec3 cameraPos;
uniform float fov;
uniform vec3 cubeCentres[100];
uniform mat4 rot;
uniform vec3 cube_size_half;
uniform int numCubes;
uniform vec3 minBound;
uniform vec3 maxBound;

const int MAX_STEPS = 100;
const float MAX_DIST = 100.0;
const float SURFACE_DIST = 0.001;

float cubeSDF(vec3 p , vec3 cubeCentre) {
    p = p - cubeCentre;
    mat3 rot3 = mat3(rot);
    vec3 rp = rot3*p;
    vec3 d = abs(rp) - cube_size_half;
    return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}

bool intersectAABB(vec3 ro, vec3 rd , vec3 minBound1, vec3 maxBound1) {
    vec3 tmin = vec3(-1e10);
    vec3 tmax = vec3(1e10);
    
    // X Component
    if (rd.x != 0.0) {
        float t1 = (minBound1.x - ro.x) / rd.x;
        float t2 = (maxBound1.x - ro.x) / rd.x;
        tmin.x = min(t1, t2);
        tmax.x = max(t1, t2);
    } else if (ro.x < minBound1.x || ro.x > maxBound1.x) {
        return false; // Ray is parallel and outside the slab
    }

    // Y Component
    if (rd.y != 0.0) {
        float t1 = (minBound1.y - ro.y) / rd.y;
        float t2 = (maxBound1.y - ro.y) / rd.y;
        tmin.y = min(t1, t2);
        tmax.y = max(t1, t2);
    } else if (ro.y < minBound1.y || ro.y > maxBound1.y) {
        return false; // Ray is parallel and outside the slab
    }

    // Z Component
    if (rd.z != 0.0) {
        float t1 = (minBound.z - ro.z) / rd.z;
        float t2 = (maxBound.z - ro.z) / rd.z;
        tmin.z = min(t1, t2);
        tmax.z = max(t1, t2);
    } else if (ro.z < minBound.z || ro.z > maxBound.z) {
        return false; // Ray is parallel and outside the slab
    }

    float tEnter = max(max(tmin.x, tmin.y), tmin.z);
    float tExit = min(min(tmax.x, tmax.y), tmax.z);

    return tEnter <= tExit && tExit > 0.0;
}

float rayMarch(vec3 ro, vec3 rd) {
    float dO = 0.0;
    for (int i = 0; i < MAX_STEPS; i++) {
        vec3 p = ro + rd * dO;
        float closestDist = 1e10;

        for (int j = 0; j < numCubes; j++) {
            if(!intersectAABB(ro , rd , cubeCentres[j]-cube_size_half , cubeCentres[j] + cube_size_half )) continue;
            float dS = cubeSDF(p, cubeCentres[j]);
            closestDist = min(closestDist, dS);
        }
        if((MAX_DIST - dO) < closestDist) discard;
        dO += closestDist;
        if (dO > MAX_DIST || closestDist < SURFACE_DIST) break;
    }
    return dO;
}




void main()
{
    vec2 uv = (gl_FragCoord.xy / iResolution.xy) * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y; // Correct aspect ratio
    vec3 rayDirCameraSpace = normalize(vec3(uv, (-1.0 / tan(radians(fov / 2.0))  )));
    mat3 invViewMatrix = mat3(transpose(viewMatrix)); // Inverse for rotation matrix
    vec3 rayDirWorldSpace = normalize(invViewMatrix * rayDirCameraSpace);

    vec3 rayOrigin = cameraPos;

    if (!intersectAABB(rayOrigin, rayDirWorldSpace , minBound , maxBound)) {
        discard; // Ray doesn't hit the world bounds, discard the fragment
    }

    float d = rayMarch(rayOrigin, rayDirWorldSpace);

    if (d < MAX_DIST) {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
