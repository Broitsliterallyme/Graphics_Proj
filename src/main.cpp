//opengl code for rendering a rotating cube
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <shader/Shader.h>
#include <shader/Compute.h>
#include<Camera.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
void processInput(GLFWwindow *window);
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
struct Voxel {
    unsigned int occupancy; // 0 = empty, nonzero = occupied.
    float R=1.0f;
    float G=1.0f;
    float B=0;

};


//Cube Length Data
float quadVertices[] = {
    // Positions (X, Y)
    -1.0f, -1.0f,  // Bottom-left
     1.0f, -1.0f,  // Bottom-right
    -1.0f,  1.0f,  // Top-left
     1.0f, -1.0f,  // Bottom-right
     1.0f,  1.0f,  // Top-right
    -1.0f,  1.0f   // Top-left
};

glm::vec3 size_half = glm::vec3(1.0f);
using namespace std;
using namespace std::chrono;

auto lastTime = high_resolution_clock::now();
int frameCount = 0;

void updateFPS() {
    frameCount++;
    auto currentTime = high_resolution_clock::now();
    duration<float> elapsed = currentTime - lastTime;

    if (elapsed.count() >= 1.0f) {
        float fps = frameCount / elapsed.count();
        cout << "FPS: " << fps << endl;
        lastTime = currentTime;
        frameCount = 0;
    }
}
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
        float getY(float x, float z, float scale = 50.0f) const {
            float amplitude = 1.0f;
            float frequency = 0.01f;
            float persistence = 0.5f;
            float lacunarity = 2.0f;
            float total = 0.0f;
            
            // Add multiple octaves for more detail
            for(int i = 0; i < 4; i++) {
                total += getHeight(x * frequency, z * frequency) * amplitude;
                amplitude *= persistence;
                frequency *= lacunarity;
            }
            
            return total * scale;
        }
    };
float generateNoise(int x, int z, float scale) {
    float noiseX = std::sin(x * scale);
    float noiseZ = std::cos(z * scale);
    
    float noiseX2 = std::sin(x * scale * 2.0f);
    float noiseZ2 = std::cos(z * scale * 2.0f);
    
    float baseNoise = (noiseX + noiseZ) * 0.5f;
    float detailNoise = (noiseX2 + noiseZ2) * 0.25f;
    
    return baseNoise + detailNoise;
}

void fillVoxelGridMountain(std::vector<Voxel>& voxelData, int gridX, int gridY, int gridZ, float scale) {
    TerrainGenerator terrainGen(0);
    // Loop over the X-Z plane.
    for (int z = 0; z < gridZ; ++z) {
        for (int x = 0; x < gridX; ++x) {
            // Generate a noise value using sine and cosine.
            float height = terrainGen.getY(x, z,scale);

            if (height > gridY) height = gridY;
            // Fill all voxels below this height as occupied (1) and the rest as empty (0).
            for (int y = 0; y < gridY; ++y) {
                int index = x + gridX * (y + gridY * z);
                voxelData[index].occupancy = (y < height) ? 1 : 0;
                if (height <= 40) {
                    // Below 40, use the base gray color.
                    voxelData[index].R = 0.3f;
                    voxelData[index].G = 0.3f;
                    voxelData[index].B = 0.3f;
                } else {
                    // For heights above 40, blend from gray to white.
                    // For example, let’s say that between height 40 and 60 the color gradually becomes white.
                    float f = (height - 40.0f) / 20.0f; // f goes from 0.0 at height==40 to 1.0 at height==60.
                    if (f > 1.0f)
                        f = 1.0f;
                    // Linearly interpolate between 0.3 (gray) and 1.0 (white) for each channel.
                    voxelData[index].R = 0.3f + f * (1.0f - 0.3f);
                    voxelData[index].G = 0.3f + f * (1.0f - 0.3f);
                    voxelData[index].B = 0.3f + f * (1.0f - 0.3f);
                }
                
            }
        }
    }
}

int main()
{
    //GLFW Init Starts
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Ray Marching Cube", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    //GLFW Init Stops

    //Shader Init Starts
    Shader ourShader("/home/erectus/Documents/Test/src/shader/vert.glsl", "/home/erectus/Documents/Test/src/shader/frag.glsl");
    ComputeShader computeShader("/home/erectus/Documents/Test/src/shader/compute.glsl");
    //Shader Init Stops

    //VAO and VBO Bind and Init Starts
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    //VAO and VBO Bind and Init Stops
    
    //Creating Texture for frag shader Starts
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 1920, 1080);
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    //Creating Texture for frag shader Ends
//Voxel location data 
// Voxel grid dimensions (increasing gridY for vertical detail)
const int gridX = 1000, gridY = 150, gridZ = 1000;
const size_t totalVoxels = gridX * gridY * gridZ;

std::vector<Voxel> voxelData(totalVoxels);
// Fill the voxel grid with random data.

// Instead of using fillVoxelGridRandom, fill the grid with mountain terrain.
// The scale value (e.g., 0.05f) and amplitude (e.g., 1.0f) can be adjusted to change the mountain shape.
fillVoxelGridMountain(voxelData, gridX, gridY, gridZ, 100.5f);

GLuint voxelSSBO;
computeShader.createSSBO(voxelSSBO, 1, voxelData.size() * sizeof(Voxel), voxelData.data(), GL_STATIC_DRAW);

Camera camera(800, 800, glm::vec3(0.0f, 0.0f, 3.0f));

    while (!glfwWindowShouldClose(window))
    {        updateFPS();
        //Processing input for window closing
        processInput(window);
        //Dispatching Compute Shader
        computeShader.dispatch(1920/16,1080/16, 1);
        computeShader.setVec2("iResolution",1920,1080);
        computeShader.setVec3("CameraPos",camera.Position.x,camera.Position.y,camera.Position.z);
        computeShader.setVec3("RayDir",camera.Orientation.x,camera.Orientation.y,camera.Orientation.z);
       // computeShader.setVec3("Sun")
        //computeShader.setVec3("cameraTarget",camera.Orientation.x,camera.Position.y,camera.Position.z);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        //Calculating and Displaying FPS Starts
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        float fps = 1.0f / deltaTime;
        std::string title = "Ray Marching Cube | FPS: " + std::to_string(fps);
        glfwSetWindowTitle(window, title.c_str());
        //Calculating and Displaying FPS Ends

        //Setting Background Color Starts
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //Setting Background Color Ends
		camera.updateMatrix(45.0f, 0.1f, 100.0f);

        //Setting Shader Uniforms Starts
        ourShader.Activate();
        //Setting Shader Uniform Ends
        
        //Sending Camera data to compute shader starts
		camera.Matrix(ourShader, "camMatrix");
		camera.Inputs(window);
        //Sending Camera data to compute shader ends
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    ourShader.Delete();
    return 0;
}


void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}


