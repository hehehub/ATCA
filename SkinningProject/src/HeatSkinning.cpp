#include "HeatSkinning.h"
#include <cmath>

static float distanceToBone(const glm::vec3 &v, const Bone &bone)
{
    glm::vec3 bonePos = glm::vec3(bone.restMatrix[3]);
    return glm::length(v - bonePos);
}

void HeatSkinning::computeWeights(
    Mesh &mesh,
    const Skeleton &skeleton)
{
    int B = skeleton.bones.size();

    for (auto &v : mesh.vertices)
    {
        std::vector<float> heat(B);

        float sum = 0.0f;
        for (int i = 0; i < B; i++)
        {
            float d = distanceToBone(v.position, skeleton.bones[i]);
            heat[i] = std::exp(-d * d); // 热传导近似
            sum += heat[i];
        }

        // 归一化 + 选 4 个最大权重
        for (float &h : heat)
            h /= sum;

        for (int k = 0; k < 4; k++)
        {
            int best = -1;
            float maxv = 0;
            for (int i = 0; i < B; i++)
            {
                if (heat[i] > maxv)
                {
                    maxv = heat[i];
                    best = i;
                }
            }
            v.boneIDs[k] = best;
            v.weights[k] = maxv;
            heat[best] = 0;
        }
    }
}
