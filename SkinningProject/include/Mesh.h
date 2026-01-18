#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;

    glm::ivec4 boneIDs = glm::ivec4(-1);
    glm::vec4 weights = glm::vec4(0.0f);
};

class Mesh
{
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    bool loadOBJ(const std::string &path);
};
