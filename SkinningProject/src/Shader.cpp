#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glad/glad.h>

Shader::Shader() : programID(0) {}

Shader::~Shader()
{
    if (programID != 0)
        glDeleteProgram(programID);
}

bool Shader::loadFromFiles(const std::string &vertexPath, const std::string &fragmentPath)
{
    std::string vertexCode, fragmentCode;
    std::ifstream vShaderFile, fShaderFile;

    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;

        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();

        vShaderFile.close();
        fShaderFile.close();

        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure &e)
    {
        std::cerr << "错误：无法读取shader文件\n" << e.what() << std::endl;
        return false;
    }

    return compile(vertexCode, fragmentCode);
}

bool Shader::compile(const std::string &vertexCode, const std::string &fragmentCode)
{
    const char *vShaderCode = vertexCode.c_str();
    const char *fShaderCode = fragmentCode.c_str();

    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    // 编译顶点shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex, 512, nullptr, infoLog);
        std::cerr << "顶点shader编译错误:\n" << infoLog << std::endl;
        return false;
    }

    // 编译片段shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, 512, nullptr, infoLog);
        std::cerr << "片段shader编译错误:\n" << infoLog << std::endl;
        glDeleteShader(vertex);
        return false;
    }

    // 创建shader程序
    programID = glCreateProgram();
    glAttachShader(programID, vertex);
    glAttachShader(programID, fragment);
    glLinkProgram(programID);

    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programID, 512, nullptr, infoLog);
        std::cerr << "Shader程序链接错误:\n" << infoLog << std::endl;
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return false;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return true;
}

void Shader::use()
{
    glUseProgram(programID);
}

void Shader::setMat4(const std::string &name, const glm::mat4 &mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setMat4Array(const std::string &name, const std::vector<glm::mat4> &mats) const
{
    glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), mats.size(), GL_FALSE, &mats[0][0][0]);
}

void Shader::setVec3(const std::string &name, const glm::vec3 &vec) const
{
    glUniform3fv(glGetUniformLocation(programID, name.c_str()), 1, &vec[0]);
}

