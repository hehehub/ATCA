#pragma once
#include <vector>
#include "Bone.h"

class Skeleton
{
public:
    std::vector<Bone> bones;

    bool loadFromJSON(const std::string &path);
    void computePoseMatrices();
};
