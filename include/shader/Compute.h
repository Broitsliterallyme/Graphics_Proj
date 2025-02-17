#ifndef COMPUTE_SHADER_H
#define COMPUTE_SHADER_H

#include <glad/glad.h>
#include <vector>
#include <string>

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

#endif
