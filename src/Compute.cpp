#include "shader/Compute.h"

ComputeShader::ComputeShader(const std::string& shaderPath) {
    // Load the shader source code from the file
    std::string shaderSource = loadShaderSource(shaderPath);

    // Create and compile the shader
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    const char* shaderSourceCStr = shaderSource.c_str();
    glShaderSource(shader, 1, &shaderSourceCStr, nullptr);
    glCompileShader(shader);
    checkShaderCompilation(shader);

    // Create the program and attach the shader
    programID = glCreateProgram();
    glAttachShader(programID, shader);
    glLinkProgram(programID);
    checkProgramLink(programID);

    // Clean up the shader as it is no longer needed after linking
    glDeleteShader(shader);
}

ComputeShader::~ComputeShader() {
    glDeleteProgram(programID);
    std::cout << "Shader program deleted successfully!" << std::endl;
}

void ComputeShader::dispatch(GLuint x, GLuint y, GLuint z) {
    glUseProgram(programID);
    glDispatchCompute(x, y, z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Ensure writes to SSBOs are completed before further usage
}

void ComputeShader::setSSBO(GLuint binding, GLuint ssbo) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, ssbo);
    std::cout << "SSBO bound successfully at binding point " << binding << std::endl;
}

void ComputeShader::createSSBO(GLuint& ssbo, GLuint binding, size_t size, void* data, GLenum usage) {
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, usage);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, ssbo);
    std::cout << "SSBO created and bound successfully at binding point " << binding << " with size " << size << "." << std::endl;
}

void ComputeShader::getSSBOData(GLuint ssbo, size_t size, void* outputBuffer) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    std::memcpy(outputBuffer, ptr, size);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    std::cout << "SSBO data read successfully from SSBO with ID " << ssbo << std::endl;
}

void ComputeShader::setVec2(const std::string& name, float x, float y) {
    GLint location = glGetUniformLocation(programID, name.c_str());
    if (location != -1) {
        glUseProgram(programID);
        glUniform2f(location, x, y);
    } else {
        std::cerr << "Warning: Uniform '" << name << "' not found!" << std::endl;
    }
}
void ComputeShader::setVec3(const std::string& name, float x, float y,float z) {
    GLint location = glGetUniformLocation(programID, name.c_str());
    if (location != -1) {
        glUseProgram(programID);
        glUniform3f(location, x, y,z);
    } else {
        std::cerr << "Warning: Uniform '" << name << "' not found!" << std::endl;
    }
}
void ComputeShader::setFloat(const std::string& name, float x) {
    GLint location = glGetUniformLocation(programID, name.c_str());
    if (location != -1) {
        glUseProgram(programID);
        glUniform1f(location, x);
    } else {
        std::cerr << "Warning: Uniform '" << name << "' not found!" << std::endl;
    }
}
void ComputeShader::setVoxelGrid(const std::string& name, const std::vector<GLint>& data) {
    GLint location = glGetUniformLocation(programID, name.c_str());
    if (location != -1) {
        glUseProgram(programID);
        glUniform1iv(location, data.size(), data.data());
    } else {
        std::cerr << "Warning: Uniform '" << name << "' not found!" << std::endl;
    }
}


std::string ComputeShader::loadShaderSource(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::cout << "Shader file '" << path << "' loaded successfully." << std::endl;
    return buffer.str();
}

void ComputeShader::checkShaderCompilation(GLuint shader) {
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength);
        glGetShaderInfoLog(shader, logLength, &logLength, log.data());
        std::cerr << "Shader compilation failed:\n" << log.data() << std::endl;
        exit(1);
    }
    std::cout << "Shader compiled successfully." << std::endl;
}

void ComputeShader::checkProgramLink(GLuint program) {
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength);
        glGetProgramInfoLog(program, logLength, &logLength, log.data());
        std::cerr << "Program linking failed:\n" << log.data() << std::endl;
        exit(1);
    }
    std::cout << "Program linked successfully." << std::endl;
}
