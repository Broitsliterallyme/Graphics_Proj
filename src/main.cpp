#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <shader/Shader.h>
#include <shader/Compute.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <random>
// A dedicated terrain noise function using basic sine/cosine waves.
float generateTerrainNoise(float x, float z) {
    // Adjust these constants to control the terrain frequency and amplitude.
    float freq = 0.05f;
    float amp  = 40.0f;
    // Noise value in range [0, amp]
    return (std::sin(x * freq) * std::cos(z * freq) + 1.0f) * 0.5f * amp;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scrool_callback(GLFWwindow* window, double xoffset, double yoffset);

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// Set camera to view a large terrain.
glm::vec3 cameraPos   = glm::vec3(50.0f, 30.0f, 120.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.3f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

bool firstMouse = true;
float yaw   = -90.0f;
float pitch = -20.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float fov   = 45.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
class TerrainGenerator {
    private:
        std::vector<int> perm;
        int seed;
    
        void initPermutation() {
            std::vector<int> p(256);
            std::iota(p.begin(), p.end(), 0);
            std::shuffle(p.begin(), p.end(), std::mt19937(seed));
            
            perm.resize(512);
            for(int i = 0; i < 512; i++)
                perm[i] = p[i & 255];
        }
    
        float fade(float t) const {
            return t * t * t * (t * (t * 6 - 15) + 10);
        }
    
        float lerp(float a, float b, float t) const {
            return a + t * (b - a);
        }
    
        float grad(int hash, float x, float z) const {
            int h = hash & 15;
            float u = h < 8 ? x : z;
            float v = h < 4 ? z : h == 12 || h == 14 ? x : 0;
            return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
        }
    
    public:
        TerrainGenerator(int seed = 0) : seed(seed) {
            initPermutation();
        }
    
        float getHeight(float x, float z) const {
            int X = (int)floor(x) & 255;
            int Z = (int)floor(z) & 255;
            x -= floor(x);
            z -= floor(z);
    
            float u = fade(x);
            float v = fade(z);
    
            int A = perm[X] + Z;
            int AA = perm[A];
            int AB = perm[A + 1];
            int B = perm[X + 1] + Z;
            int BA = perm[B];
            int BB = perm[B + 1];
    
            return lerp(lerp(grad(AA, x,   z),
                           grad(BA, x-1, z),
                           u),
                    lerp(grad(AB, x,   z-1),
                           grad(BB, x-1, z-1),
                           u),
                    v);
        }
    
        // Example scaling function to convert noise to terrain height
        float getY(float x, float z, float scale = 60.0f) const {
            float amplitude = 2.5f;
            float frequency = 0.005f;
            float persistence = 0.4f;
            float lacunarity = 1.0f;
            float total = 0.0f;
            
            // Add multiple octaves for more detail
            for(int i = 0; i < 6; i++) {
                total += getHeight(x * frequency, z * frequency) * amplitude;
                amplitude *= persistence;
                frequency *= lacunarity;
            }
            
            return total * scale;
        }
    };
  // Structure representing a color stop.
struct ColorStop {
    float height;      // The elevation at which this color applies.
    glm::vec4 color;   // The color at this height.
};
std::vector<ColorStop> mountainStops = {
    { 50.0f, glm::vec4(0.1f, 0.3f, 0.1f, 1.0f) }, // Lower altitudes: lush green
    { 100.0f, glm::vec4(0.1f, 0.2f, 0.1f, 1.0f) }, // Transition: gray for rocky areas
    { 200.0f, glm::vec4(0.6f, 0.6f, 0.6f, 1.0f) }, // Higher altitudes: light gray
    { 300.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) }  // Peaks: snow white
};
// Given a noiseHeight and a sorted array of color stops, returns the interpolated color.
glm::vec4 getMountainColor(float noiseHeight, const std::vector<ColorStop>& stops) {
    // If no stops are provided, return white as a fallback.
    if (stops.empty())
        return glm::vec4(1.0f);

    // If below the first stop, return the first color.
    if (noiseHeight <= stops.front().height)
        return stops.front().color;
    
    // If above the last stop, return the last color.
    if (noiseHeight >= stops.back().height)
        return stops.back().color;
    
    // Find the two stops noiseHeight lies between.
    for (size_t i = 0; i < stops.size() - 1; i++) {
        if (noiseHeight < stops[i + 1].height) {
            float t = (noiseHeight - stops[i].height) / (stops[i + 1].height - stops[i].height);
            return glm::mix(stops[i].color, stops[i + 1].color, t);
        }
    }
    
    return stops.back().color; // Fallback (should not be reached)
}
struct FlattenedNode {
    bool IsLeaf = false;
    char padding1[3];       // Pad bool to 4 bytes
    int childIndices[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    char padding2[12];      // Pad to align vec4 to 16 bytes after the array
    glm::vec4 color = glm::vec4(1.0f); // default white
};

std::vector<FlattenedNode> m_nodes;

class SparseVoxelOctree {
public:
    SparseVoxelOctree(int size, int maxDepth);
    void Insert(glm::vec3 point, glm::vec4 color);
private:
    void InsertImpl(int nodeIndex, glm::ivec3 point, glm::vec4 color, glm::ivec3 position, int depth);
    int m_size;
    int m_maxDepth;
};

SparseVoxelOctree::SparseVoxelOctree(int size, int maxDepth)
    : m_size(size), m_maxDepth(maxDepth) {
    m_nodes.push_back(FlattenedNode()); // root node
}

void SparseVoxelOctree::Insert(glm::vec3 point, glm::vec4 color) {
    InsertImpl(0, glm::ivec3(point), color, glm::ivec3(0), 0);
}

void SparseVoxelOctree::InsertImpl(int nodeIndex, glm::ivec3 point, glm::vec4 color, glm::ivec3 position, int depth) {
    if (nodeIndex >= m_nodes.size()) {
        std::cout << "Index out of bounds" << std::endl;
        return;
    }
    FlattenedNode &node = m_nodes[nodeIndex];
    node.color = color;
    if (depth == m_maxDepth) {
        node.IsLeaf = true;
        return;
    }
    float size = m_size / std::exp2(depth);
    glm::ivec3 center = position + glm::ivec3(size / 2.0f);
    glm::ivec3 childPos = {
        (point.x >= center.x) ? 1 : 0,
        (point.y >= center.y) ? 1 : 0,
        (point.z >= center.z) ? 1 : 0
    };
    int childIndex = (childPos.x << 2) | (childPos.y << 1) | (childPos.z);
    if (node.childIndices[childIndex] == -1) {
        node.childIndices[childIndex] = m_nodes.size();
        m_nodes.push_back(FlattenedNode());
    }
    glm::ivec3 newPosition = position + childPos * glm::ivec3(size / 2);
    InsertImpl(node.childIndices[childIndex], point, color, newPosition, depth + 1);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
TerrainGenerator terrainGen(20);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Terrain Generation", NULL, NULL);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    Shader ourShader("/home/erectus/Documents/Octree/src/shader/vert.glsl", "/home/erectus/Documents/Octree/src/shader/frag.glsl");
    ComputeShader computeShader("/home/erectus/Documents/Octree/src/shader/compute.glsl");

    float quadVertices[] = {
        -1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };

// Setup octree for terrain.
    int octreeSize = 1550;  // The world spans from 0 to 100 along x and z.
    int maxDepth = 9;      // Adjust as needed; note that higher depths yield smaller voxels.
    SparseVoxelOctree octree(octreeSize, maxDepth);
    m_nodes.reserve(50000000); // Pre-reserve enough memory to reduce reallocations.

// Compute the current voxel size.
float voxelSize = static_cast<float>(octreeSize) / std::exp2(maxDepth);

    // Generate terrain: for each (x, z) coordinate, compute a terrain height using noise,
    // then fill the column from y = 0 up to that height, stepping in increments of voxelSize.
    // Compute how many voxels we have along one axis
int voxelsPerAxis = static_cast<int>(octreeSize / voxelSize);

for (int ix = 0; ix < voxelsPerAxis; ix++) {
    float x = ix * voxelSize;
    for (int iz = 0; iz < voxelsPerAxis; iz++) {
        float z = iz * voxelSize;
        
        // Compute the terrain height at this (x, z) location.
        float noiseHeight = terrainGen.getY(x, z); // returns height in world units
        
        // Compute number of voxels in y-direction based on noise height
        
        // Precompute the color for this column once
        glm::vec4 color = getMountainColor(noiseHeight, mountainStops);

        
        
        // Insert voxels for each y-level

            glm::vec3 pos(x, noiseHeight, z);
            glm::vec3 pos1(x, noiseHeight-1, z);
            glm::vec3 pos2(x, noiseHeight-2, z);
            glm::vec3 pos3(x,noiseHeight-3,z);
            octree.Insert(pos, color);
            octree.Insert(pos1, color);
            octree.Insert(pos2, color);
            octree.Insert(pos3,color);
        
    }
}



    glm::vec3 minBound = glm::vec3(0, 0, 0);
    glm::vec3 maxBound = glm::vec3(octreeSize, octreeSize, octreeSize);

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, SCR_WIDTH, SCR_HEIGHT);
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_nodes.size() * sizeof(FlattenedNode), m_nodes.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);
// Query the size of the SSBO
GLint ssboSize = 0;
glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, &ssboSize);
std::cout << "SSBO size: " << ssboSize << " bytes" << std::endl;
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        computeShader.use();
        float cycleDuration = 10.0f; // seconds
        float globalTime =currentFrame; // your time in seconds
        float timeOfDay = fmod(globalTime, cycleDuration) / cycleDuration;
        
        computeShader.setMat4("viewMatrix", glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp));
        computeShader.setVec3("cameraPos", cameraPos);
        computeShader.setFloat("fov", fov);
        computeShader.setVec2("iResolution", SCR_WIDTH, SCR_HEIGHT);
        computeShader.setVec3("minBound", minBound);
        computeShader.setVec3("maxBound", maxBound);
        computeShader.setFloat("timeOfDay", timeOfDay);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);
        computeShader.dispatch((SCR_WIDTH + 15) / 16, (SCR_HEIGHT + 15) / 16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetScrollCallback(window, scrool_callback);

        float fps = 1.0f / deltaTime;
std::cout << fps << std::endl;

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    float cameraSpeed = 40.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraUp;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    static bool firstMouse = true;
    static float lastX = SCR_WIDTH / 2.0f;
    static float lastY = SCR_HEIGHT / 2.0f;
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }
    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);
    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    yaw   += xoffset;
    pitch += yoffset;
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void scrool_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (fov >= 1.0f && fov <= 45.0f)
        fov -= static_cast<float>(yoffset);
    if (fov <= 1.0f)
        fov = 1.0f;
    if (fov >= 45.0f)
        fov = 45.0f;
}