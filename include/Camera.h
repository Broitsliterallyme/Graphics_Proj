#define GLM_ENABLE_EXPERIMENTAL
#include<glm/glm.hpp>
#include<glad/glad.h>
#include <GLFW/glfw3.h>
#include<shader/Shader.h>
#include<glm/gtc/type_ptr.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtx/rotate_vector.hpp>
#include<glm/gtx/vector_angle.hpp>


class Camera {
public:
	float deltaTime;
	float lastTime;
	glm::vec3 Position;
	glm::vec3 Orientation = glm::vec3(0.0f, 0.0f, -0.5f);
	glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::mat4 CameraMatrix = glm::mat4(1.0f);  
	int Width;
	int Height;
	float speed = 1.0f;
	float sensitivity = 100.0f;
	Camera(int width, int height, glm::vec3 position);
	void updateMatrix(float FOVdeg, float nearPlane, float farPlane);
	void Matrix( Shader& shader, const char* uniform);
	void Inputs(GLFWwindow* window);
};