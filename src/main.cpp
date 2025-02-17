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

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
float deltaTime = 0.0f;
float lastFrame = 0.0f;


//Cube Length Data
float quadVertices[] = {
    // positions
    -1.0f,  1.0f,
    -1.0f, -1.0f,
     1.0f, -1.0f,

    -1.0f,  1.0f,
     1.0f, -1.0f,
     1.0f,  1.0f
};
glm::vec3 size_half = glm::vec3(1.0f);

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
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
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
	Camera camera(800, 800, glm::vec3(0.0f, 0.0f, 3.0f));
    while (!glfwWindowShouldClose(window))
    {
        //Processing input for window closing
        processInput(window);

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

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}


