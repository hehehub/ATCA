#include "Skeleton.h"
#include "json.hpp"
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>

using json = nlohmann::json;

bool Skeleton::loadFromJSON(const std::string &path)
{
    std::ifstream f(path);
    json j;
    f >> j;

    std::map<std::string, int> nameToIndex;

    // 第一遍：创建所有骨骼并建立名称映射
    for (size_t i = 0; i < j.size(); i++)
    {
        auto &b = j[i];
        Bone bone;
        bone.id = i;
        bone.name = b["name"];
        nameToIndex[bone.name] = i;

        // 从head/tail构建矩阵
        glm::vec3 head(b["head"][0], b["head"][1], b["head"][2]);
        glm::vec3 tail(b["tail"][0], b["tail"][1], b["tail"][2]);
        glm::vec3 dir = glm::normalize(tail - head);

        // 构建局部坐标系矩阵
        glm::vec3 up(0, 1, 0);
        if (glm::abs(glm::dot(dir, up)) > 0.9f)
            up = glm::vec3(1, 0, 0);

        glm::vec3 right = glm::normalize(glm::cross(dir, up));
        up = glm::normalize(glm::cross(right, dir));

        bone.restMatrix = glm::mat4(
            glm::vec4(right, 0),
            glm::vec4(up, 0),
            glm::vec4(dir, 0),
            glm::vec4(head, 1));
        bone.invRestMatrix = glm::inverse(bone.restMatrix);

        bones.push_back(bone);
    }

    // 第二遍：设置parent索引
    for (size_t i = 0; i < j.size(); i++)
    {
        auto &b = j[i];
        if (b["parent"].is_null())
        {
            bones[i].parent = -1;
        }
        else
        {
            std::string parentName = b["parent"];
            bones[i].parent = nameToIndex[parentName];
        }
    }

    return true;
}

void Skeleton::computePoseMatrices()
{
    for (size_t i = 0; i < bones.size(); i++)
    {
        if (bones[i].parent == -1)
        {
            // Root bone: pose = rest * animation
            bones[i].poseMatrix = bones[i].restMatrix;
        }
        else
        {
            // Child bone: pose = parent_pose * (parent_inv_rest * rest) * animation
            bones[i].poseMatrix = bones[bones[i].parent].poseMatrix *
                                  bones[bones[i].parent].invRestMatrix *
                                  bones[i].restMatrix;
        }
    }
}
