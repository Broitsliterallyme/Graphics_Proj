#ifndef COMPUTE_SHADER_H
#define COMPUTE_SHADER_H

#include <glad/glad.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
class ComputeShader {
public:
    ComputeShader(const std::string& shaderPath);
    ~ComputeShader();
    void use();
    void dispatch(GLuint x, GLuint y = 1, GLuint z = 1);
    void setSSBO(GLuint binding, GLuint ssbo);
    void createSSBO(GLuint& ssbo, GLuint binding, size_t size, void* data = nullptr, GLenum usage = GL_DYNAMIC_DRAW);
    void getSSBOData(GLuint ssbo, size_t size, void* outputBuffer);

    void setBool(const std::string &name, bool value) const {
        //glUseProgram(programID); 
        glUniform1i(glGetUniformLocation(programID, name.c_str()), (int)value);
        //glUseProgram(0); 
    }
    
    void setInt(const std::string &name, int value) const {
        //glUseProgram(programID);
        glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
        //glUseProgram(0);
    }
    
    void setFloat(const std::string &name, float value) const {
        //glUseProgram(programID);
        glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
        //glUseProgram(0);
    }
    
    void setVec3(const std::string &name, const glm::vec3 &value) const {
        //glUseProgram(programID);
        glUniform3fv(glGetUniformLocation(programID, name.c_str()), 1, &value[0]);
        //glUseProgram(0);
    }

    void setVec2(const std::string &name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(programID, name.c_str()), x, y);
    }
    
    void setMat4(const std::string &name, const glm::mat4 &mat) const {
        //glUseProgram(programID);
        glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        //glUseProgram(0);
    }
    

private:
    GLuint shaderID;
    GLuint programID;

    std::string loadShaderSource(const std::string& path);
    void checkShaderCompilation(GLuint shader);
    void checkProgramLink(GLuint program);
};


ComputeShader::ComputeShader(const std::string& shaderPath) {
    std::string source = loadShaderSource(shaderPath);
    shaderID = glCreateShader(GL_COMPUTE_SHADER);
    const char* sourceCStr = source.c_str();
    glShaderSource(shaderID, 1, &sourceCStr, nullptr);
    glCompileShader(shaderID);
    checkShaderCompilation(shaderID);

    programID = glCreateProgram();
    glAttachShader(programID, shaderID);
    glLinkProgram(programID);
    checkProgramLink(programID);
}

ComputeShader::~ComputeShader() {
    glDeleteProgram(programID);
    glDeleteShader(shaderID);
}

void ComputeShader::use() {
    glUseProgram(programID);
}

void ComputeShader::dispatch(GLuint x, GLuint y, GLuint z) {
    glDispatchCompute(x, y, z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void ComputeShader::setSSBO(GLuint binding, GLuint ssbo) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, ssbo);
}

void ComputeShader::createSSBO(GLuint& ssbo, GLuint binding, size_t size, void* data, GLenum usage) {
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, usage);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, ssbo);
}

void ComputeShader::getSSBOData(GLuint ssbo, size_t size, void* outputBuffer) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    if (ptr) {
        memcpy(outputBuffer, ptr, size);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
}

std::string ComputeShader::loadShaderSource(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void ComputeShader::checkShaderCompilation(GLuint shader) {
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Compute Shader Compilation Error:\n" << log << std::endl;
    }
}

void ComputeShader::checkProgramLink(GLuint program) {
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cerr << "Shader Program Linking Error:\n" << log << std::endl;
    }
}

#endif