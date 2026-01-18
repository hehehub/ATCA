#include "Mesh.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool Mesh::loadOBJ(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "无法打开文件: " << path << std::endl;
        return false;
    }

    std::vector<glm::vec3> temp_positions;
    std::vector<glm::vec3> temp_normals;
    std::vector<glm::vec2> temp_texcoords;
    
    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v")
        {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            temp_positions.push_back(pos);
        }
        else if (type == "vn")
        {
            glm::vec3 norm;
            iss >> norm.x >> norm.y >> norm.z;
            temp_normals.push_back(norm);
        }
        else if (type == "vt")
        {
            glm::vec2 tex;
            iss >> tex.x >> tex.y;
            temp_texcoords.push_back(tex);
        }
        else if (type == "f")
        {
            std::vector<std::string> tokens;
            std::string token;
            while (iss >> token)
                tokens.push_back(token);

            // 处理面，支持 v/vt/vn 格式
            for (const auto &t : tokens)
            {
                std::istringstream tokenStream(t);
                std::string v_str, vt_str, vn_str;
                
                std::getline(tokenStream, v_str, '/');
                std::getline(tokenStream, vt_str, '/');
                std::getline(tokenStream, vn_str, '/');

                int v_idx = std::stoi(v_str) - 1; // OBJ索引从1开始
                int vn_idx = vn_str.empty() ? -1 : std::stoi(vn_str) - 1;

                Vertex vertex;
                if (v_idx >= 0 && v_idx < (int)temp_positions.size())
                    vertex.position = temp_positions[v_idx];
                
                if (vn_idx >= 0 && vn_idx < (int)temp_normals.size())
                    vertex.normal = temp_normals[vn_idx];
                else
                    vertex.normal = glm::vec3(0, 1, 0); // 默认法线

                vertices.push_back(vertex);
                indices.push_back(vertices.size() - 1);
            }
        }
    }

    // 如果没有法线，计算面法线
    if (temp_normals.empty())
    {
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            if (i + 2 < indices.size())
            {
                Vertex &v0 = vertices[indices[i]];
                Vertex &v1 = vertices[indices[i + 1]];
                Vertex &v2 = vertices[indices[i + 2]];

                glm::vec3 edge1 = v1.position - v0.position;
                glm::vec3 edge2 = v2.position - v0.position;
                glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

                v0.normal += normal;
                v1.normal += normal;
                v2.normal += normal;
            }
        }

        // 归一化法线
        for (auto &v : vertices)
            v.normal = glm::normalize(v.normal);
    }

    return true;
}

