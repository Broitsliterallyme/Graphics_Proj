//opengl code for rendering a rotating cube
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <shader/shader_m.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scrool_callback(GLFWwindow* window, double xoffset, double yoffset);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
glm::vec3 newPos = cameraPos;
bool collision = false;

bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float fov = 45.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
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

    glEnable(GL_DEPTH_TEST);

    Shader ourShader("/home/erectus/Documents/Graphics_Proj/src/shader/Vert.glsl", "/home/erectus/Documents/Graphics_Proj/src/shader/Frag.glsl");
    ComputeShader compute("/home/erectus/Documents/Graphics_Proj/src/shader/compute_shader.glsl");

    float quadVertices[] = {
        // positions
        -1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f, -1.0f,

        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };

    glm::vec3 translations[100];
    // translations[0] = glm::vec3(0.0f, 0.0f, 0.0f);
    // translations[1] = glm::vec3(2.5f, 2.5f, 0.0f);
    int index = 0;
    float offset = 2.5f;
    for(int y = -5; y < 5; y += 1)
    {
        for(int x = -5; x < 5; x += 1)
        {
            glm::vec2 translation;
            translation.x = (float)x  * offset;
            translation.y = (float)y  * offset;
            translations[index++] = glm::vec3(translation,0);
        }
    }
    
    glm::vec3 minBound = glm::vec3(-11.0f, -11.0f, 1.0f);
    glm::vec3 maxBound = glm::vec3(9.0f, 9.0f, -1.0f);


    glm::vec3 size_half = glm::vec3(1.0f);

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

    GLuint ssbo;
    float data[5] = {1, 2, 3, 4, 5};
    compute.createSSBO(ssbo, 0, sizeof(data), data);


    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);

        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetScrollCallback(window, scrool_callback);

        //for showing fps on top
        float fps = 1.0f / deltaTime;
        std::string title = "Ray Marching Cube | FPS: " + std::to_string(fps);
        glfwSetWindowTitle(window, title.c_str());


        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();
        for(unsigned int i = 0; i < 100; i++)
        {
            ourShader.setVec3((("cubeCentres[" + std::to_string(i) + "]")), translations[i]);
        } 
        compute.dispatch(5);

        glm::mat4 view = glm::lookAt(glm::vec3(0.0f , 0.0f , 0.0f), cameraFront, cameraUp);
        ourShader.setInt("numCubes", 100);
        ourShader.setMat4("viewMatrix", view);
        ourShader.setVec3("cameraPos", cameraPos);
        ourShader.setFloat("fov", fov);
        ourShader.setVec2("iResolution", SCR_WIDTH, SCR_HEIGHT);
        // ourShader.setVec3("cubeCentre", centre);
        ourShader.setVec3("cameraFront", cameraFront);
        ourShader.setVec3("cube_size_half", size_half);
        ourShader.setVec3("minBound", minBound);
        ourShader.setVec3("maxBound", maxBound);
        glm::mat4 trans = glm::mat4(1.0f);
        trans = glm::rotate(trans , glm::radians(0.0f) , glm::vec3(1.0f, 0.0f, 1.0f));
        ourShader.setMat4("rot", trans);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // glm::vec3 localPos = glm::mat3(trans) * (newPos - centre);

    
        // glm::vec3 d = glm::abs(localPos) - size_half;
        // float distance = glm::length(glm::max(d, glm::vec3(0.0f))) + 
        //                 glm::min(glm::max(d.x, glm::max(d.y, d.z)), 0.0f);

        //std::cout<<newPos.x<<" "<<newPos.y<<" "<<newPos.z<<std::endl;

        // For debugging:
        // std::cout<<cameraPos.x<<" "<<cameraPos.y<<" "<<cameraPos.z<<std::endl;
        // std::cout<<"Camera Front"<<std::endl;
        //std::cout<<cameraFront.x<<" "<<cameraFront.y<<" "<<cameraFront.z<<std::endl;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = static_cast<float>(2.5 * deltaTime);
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

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    static bool firstMouse = true;
    static float lastX = SCR_WIDTH / 2.0f;
    static float lastY = SCR_HEIGHT / 2.0f;

    if (firstMouse)
    {
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

    yaw += xoffset;
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

void scrool_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (fov >= 1.0f && fov <= 45.0f)
        fov -= static_cast<float>(yoffset);
    if (fov <= 1.0f)
        fov = 1.0f;
    if (fov >= 45.0f)
        fov = 45.0f;
}

