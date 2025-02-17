cmake_minimum_required(VERSION 3.10)
project(OpenGLExample)
# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# Add OpenGL library
find_package(OpenGL REQUIRED)
# Add GLFW
find_package(glfw3 REQUIRED)
#Add glm
find_package(glm REQUIRED)
# Include GLAD manually
add_library(GLAD STATIC src/glad.c)
target_include_directories(GLAD PUBLIC include/)
# Add your source files
add_executable(OpenGLExample src/main.cpp)
# Link libraries
target_link_libraries(OpenGLExample PRIVATE OpenGL::GL glfw GLAD)