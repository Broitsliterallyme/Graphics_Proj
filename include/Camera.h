#define GLM_ENABLE_EXPERIMENTAL
#include<glm/glm.hpp>
#include<glad/glad.h>
#include <GLFW/glfw3.h>
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
Camera::Camera(int width, int height, glm::vec3 position) {
	Width = width;
	Height = height;
	Position = position;
}
void Camera::updateMatrix(float FOVdeg, float nearPlane, float farPlane) {
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);
	view = glm::lookAt(Position, Position + Orientation, Up);
	projection = glm::perspective(glm::radians(FOVdeg), (float)Width / (float)Height, nearPlane, farPlane);
	CameraMatrix = projection * view;
}
void Camera::Matrix(Shader& shader, const char* uniform) {
	shader.setMat4(uniform, CameraMatrix);
}
void Camera::Inputs(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		Position += speed * Orientation;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		Position -= speed * Orientation;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		Position -= glm::normalize(glm::cross(Orientation, Up)) * speed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		Position += glm::normalize(glm::cross(Orientation, Up)) * speed;
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		Position += Up * speed;
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		Position -= Up * speed;
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		speed = 0.1f;
	else
		speed = 0.01f;
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	double mouseX;
	double mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);
	float rotX = sensitivity * (float)(mouseY - (Height / 2)) / Height ;
	float rotY = sensitivity * (float)(mouseX - (Width / 2)) / Width;
	glm::vec3 newOrientation = glm::rotate(Orientation, glm::radians(-rotX), glm::normalize(glm::cross(Orientation, Up)));
	if (abs(glm::angle(newOrientation, Up) - glm::radians(90.0f)) <= glm::radians(85.0f))
	{
		Orientation = newOrientation;
	}
	Orientation = glm::rotate(Orientation, glm::radians(-rotY), Up);
	glfwSetCursorPos(window, (Width / 2), (Height / 2));


	//Right = glm::normalize(glm::cross(Orientation, Up));



}
