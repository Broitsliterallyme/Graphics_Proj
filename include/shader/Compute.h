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

void ComputeShader::dispatch(GLuint x, GLuint y, GLuint z) {
    glUseProgram(programID);
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
