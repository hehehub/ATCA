#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in ivec4 aBoneIDs;
layout (location = 3) in vec4 aWeights;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uBoneMatrices[200]; // 最多200个骨骼

out vec3 FragPos;
out vec3 Normal;

void main()
{
    // 使用热传导蒙皮：根据顶点权重混合多个骨骼变换
    mat4 boneTransform = mat4(0.0);
    
    // 遍历4个骨骼权重，只使用有效的骨骼ID（>= 0）和权重（> 0）
    for (int i = 0; i < 4; i++)
    {
        if (aBoneIDs[i] >= 0 && aWeights[i] > 0.0)
        {
            // 矩阵线性组合：boneMatrix * weight
            boneTransform += uBoneMatrices[aBoneIDs[i]] * aWeights[i];
        }
    }
    
    // 如果所有权重都为0或骨骼ID无效，使用单位矩阵（不变换）
    if (boneTransform == mat4(0.0))
    {
        boneTransform = mat4(1.0);
    }

    vec4 worldPos = uModel * boneTransform * vec4(aPosition, 1.0);
    FragPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(uModel * boneTransform))) * aNormal;
    gl_Position = uProjection * uView * worldPos;
}