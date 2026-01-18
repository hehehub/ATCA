#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>

class Shader
{
public:
    Shader();
    ~Shader();

    bool loadFromFiles(const std::string &vertexPath, const std::string &fragmentPath);
    bool compile(const std::string &vertexCode, const std::string &fragmentCode);
    void use();
    void setMat4(const std::string &name, const glm::mat4 &mat) const;
    void setMat4Array(const std::string &name, const std::vector<glm::mat4> &mats) const;
    void setVec3(const std::string &name, const glm::vec3 &vec) const;

    unsigned int getID() const { return programID; }

private:
    unsigned int programID;
};

