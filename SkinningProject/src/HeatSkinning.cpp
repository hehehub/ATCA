#include "HeatSkinning.h"
#include <cmath>
#include <algorithm>

static float distanceToBoneSegment(
    const glm::vec3 &v,
    const Skeleton &skeleton,
    int boneID)
{
    int parent = skeleton.bones[boneID].parent;
    if (parent < 0)
    {
        glm::vec3 p = glm::vec3(skeleton.bones[boneID].restMatrix[3]);
        return glm::length(v - p);
    }

    glm::vec3 p0 = glm::vec3(skeleton.bones[parent].restMatrix[3]);
    glm::vec3 p1 = glm::vec3(skeleton.bones[boneID].restMatrix[3]);

    glm::vec3 d = p1 - p0;
    float len2 = glm::dot(d, d);
    if (len2 < 1e-6f)
        return glm::length(v - p0);

    float t = glm::dot(v - p0, d) / len2;
    t = glm::clamp(t, 0.0f, 1.0f);
    return glm::length(v - (p0 + t * d));
}

// ------------------
// 判断下半身骨骼，只包含大腿和小腿
// ------------------
static bool isLegBone(int i)
{
    return (i == 149 || i == 150 || i == 154 || i == 155); // thigh.L, shin.L, thigh.R, shin.R
}

void HeatSkinning::computeWeights(Mesh &mesh, const Skeleton &skeleton)
{
    const int B = skeleton.bones.size();
    const float centerX = skeleton.bones[147].restMatrix[3].x;

    for (auto &v : mesh.vertices)
    {
        std::vector<float> heat(B, 0.0f);

        for (int i = 0; i < B; i++)
        {
            // 1. 核心骨骼分类
            bool isSpine = (i <= 6); // 包含所有 spine 链
            bool isPelvis = (i == 147 || i == 148);
            bool isThigh = (i == 149 || i == 154);
            bool isShin = (i == 150 || i == 155);

            // 2. 检测脚部相关骨骼 (151-153 左, 156-158 右)
            bool isLeftFoot = (i >= 151 && i <= 153);
            bool isRightFoot = (i >= 156 && i <= 158);

            // 3. 只有躯干和腿部骨骼参与计算，其他的全部跳过
            if (!isSpine && !isPelvis && !isThigh && !isShin && !isLeftFoot && !isRightFoot)
                continue;

            // 4. 左右隔离：左腿顶点不看右腿骨骼，反之亦然
            if (v.position.x < centerX - 0.1f && (i == 154 || i == 155 || isRightFoot))
                continue;
            if (v.position.x > centerX + 0.1f && (i == 149 || i == 150 || isLeftFoot))
                continue;

            float d = distanceToBoneSegment(v.position, skeleton, i);
            float falloff = 0.1f;
            float h = std::exp(-(d * d) / falloff);

            // 5. 【关键点】权重重定向：如果算出来是脚的权重，直接加给小腿
            if (isLeftFoot)
            {
                heat[150] += h * 0.5f; // 150 是 shin.L
            }
            else if (isRightFoot)
            {
                heat[155] += h * 0.5f; // 155 是 shin.R
            }
            else
            {
                // 正常的权重分配
                if (isSpine || isPelvis)
                    h *= 2.0f; // 增强躯干拉力
                heat[i] += h;
            }
        }

        // --- 归一化与选取最大 4 个权重 ---
        float sum = 0.0f;
        for (float h : heat)
            sum += h;

        // 如果该顶点距离所有核心骨骼都太远，强制绑定到最近的 spine 或 pelvis
        if (sum < 1e-6f)
        {
            v.boneIDs[0] = 0;
            v.weights[0] = 1.0f;
            for (int k = 1; k < 4; k++)
            {
                v.boneIDs[k] = 0;
                v.weights[k] = 0.0f;
            }
            continue;
        }

        for (int k = 0; k < 4; k++)
        {
            int best = -1;
            float maxv = -1.0f;
            for (int i = 0; i < B; i++)
            {
                if (heat[i] > maxv)
                {
                    maxv = heat[i];
                    best = i;
                }
            }
            if (best >= 0 && maxv > 1e-9f)
            {
                v.boneIDs[k] = best;
                v.weights[k] = maxv;
                heat[best] = -1.0f;
            }
            else
            {
                v.boneIDs[k] = 0;
                v.weights[k] = 0.0f;
            }
        }

        // 最终归一化，确保顶点受力平衡
        float finalSum = v.weights[0] + v.weights[1] + v.weights[2] + v.weights[3];
        if (finalSum > 0)
        {
            for (int k = 0; k < 4; k++)
                v.weights[k] /= finalSum;
        }
    }
}
