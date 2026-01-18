#pragma once
#include <glm/glm.hpp>
#include <string>

struct Bone
{
    int id;
    int parent;              // 父骨骼索引，-1 表示 root
    glm::mat4 restMatrix;    // 静止姿态矩阵
    glm::mat4 invRestMatrix; // 静止姿态逆矩阵
    glm::mat4 poseMatrix;    // 当前动画姿态
    std::string name;
};
