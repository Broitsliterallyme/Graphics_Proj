#ifndef COMPUTE_SHADER_H
#define COMPUTE_SHADER_H

#include <glad/glad.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
class ComputeShader {
public:
    ComputeShader(const std::string& shaderPath);
    ~ComputeShader();

    void dispatch(GLuint x, GLuint y = 1, GLuint z = 1);
    void setSSBO(GLuint binding, GLuint ssbo);
    void createSSBO(GLuint& ssbo, GLuint binding, size_t size, void* data = nullptr, GLenum usage = GL_DYNAMIC_DRAW);
    void getSSBOData(GLuint ssbo, size_t size, void* outputBuffer);
    void setVec2(const std::string& name, float x, float y);
    void setVec3(const std::string& name, float x, float y,float z);
    void setFloat(const std::string& name, float x);
    private:
    GLuint shaderID;
    GLuint programID;

    std::string loadShaderSource(const std::string& path);
    void checkShaderCompilation(GLuint shader);
    void checkProgramLink(GLuint program);
};


#endif
