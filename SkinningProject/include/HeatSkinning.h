#pragma once
#include "Mesh.h"
#include "Skeleton.h"

class HeatSkinning
{
public:
    static void computeWeights(
        Mesh &mesh,
        const Skeleton &skeleton);
};
