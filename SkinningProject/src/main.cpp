#include "Skeleton.h"
#include "Mesh.h"
#include "Shader.h"
#include "HeatSkinning.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>

// 窗口大小
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const float FPS = 30.0f;
const float DURATION = 10.0f; // 10秒视频
const int TOTAL_FRAMES = (int)(FPS * DURATION);

GLFWwindow *window = nullptr;
Shader shader;
Mesh mesh;
Skeleton skeleton;
unsigned int VAO, VBO, EBO;

void buildGlobalPose(int boneIdx, Skeleton &skeleton)
{
    int parent = skeleton.bones[boneIdx].parent;
    if (parent != -1)
    {
        skeleton.bones[boneIdx].poseMatrix =
            skeleton.bones[parent].poseMatrix *
            skeleton.bones[parent].invRestMatrix *
            skeleton.bones[boneIdx].poseMatrix;
    }

    for (int i = 0; i < (int)skeleton.bones.size(); ++i)
    {
        if (skeleton.bones[i].parent == boneIdx)
        {
            buildGlobalPose(i, skeleton);
        }
    }
}

// 动画函数：简单的行走动画
void updateWalkingAnimation(float time, Skeleton &skeleton)
{
    // 1. 重置所有骨骼到 rest pose
    for (auto &b : skeleton.bones)
        b.poseMatrix = b.restMatrix;

    // 2. 找关键骨骼
    int root = -1;
    int thighL = -1, shinL = -1;
    int thighR = -1, shinR = -1;

    for (int i = 0; i < (int)skeleton.bones.size(); ++i)
    {
        const std::string &n = skeleton.bones[i].name;
        if (n == "spine")
            root = i;
        else if (n == "thigh.L")
            thighL = i;
        else if (n == "shin.L")
            shinL = i;
        else if (n == "thigh.R")
            thighR = i;
        else if (n == "shin.R")
            shinR = i;
    }

    if (thighL < 0 || shinL < 0 || thighR < 0 || shinR < 0 || root < 0)
    {
        std::cout << "leg or root bones not found\n";
        return;
    }

    // 3. 根骨固定，不随腿摆动
    skeleton.bones[root].poseMatrix = skeleton.bones[root].restMatrix;

    // 4. 行走相位
    float phase = std::sin(time * 2.0f);

    // 5. 只改变腿骨局部旋转
    skeleton.bones[thighL].poseMatrix =
        skeleton.bones[thighL].restMatrix *
        glm::rotate(glm::mat4(1.0f), phase * 0.6f, glm::vec3(1, 0, 0));

    skeleton.bones[shinL].poseMatrix =
        skeleton.bones[shinL].restMatrix *
        glm::rotate(glm::mat4(1.0f), std::max(0.0f, -phase) * 0.8f, glm::vec3(1, 0, 0));

    skeleton.bones[thighR].poseMatrix =
        skeleton.bones[thighR].restMatrix *
        glm::rotate(glm::mat4(1.0f), -phase * 0.6f, glm::vec3(1, 0, 0));

    skeleton.bones[shinR].poseMatrix =
        skeleton.bones[shinR].restMatrix *
        glm::rotate(glm::mat4(1.0f), std::max(0.0f, phase) * 0.8f, glm::vec3(1, 0, 0));
}

// 保存帧到文件
void saveFrame(const std::string &filename, int frameWidth, int frameHeight)
{
    std::vector<unsigned char> pixels(frameWidth * frameHeight * 3);
    glReadPixels(0, 0, frameWidth, frameHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // 翻转Y轴（OpenGL的坐标系是上下颠倒的）
    std::vector<unsigned char> flipped(frameWidth * frameHeight * 3);
    for (int y = 0; y < frameHeight; y++)
    {
        for (int x = 0; x < frameWidth; x++)
        {
            int srcIndex = (y * frameWidth + x) * 3;
            int dstIndex = ((frameHeight - 1 - y) * frameWidth + x) * 3;
            flipped[dstIndex] = pixels[srcIndex];
            flipped[dstIndex + 1] = pixels[srcIndex + 1];
            flipped[dstIndex + 2] = pixels[srcIndex + 2];
        }
    }

    // 保存为PPM格式（简单格式，可以用FFmpeg转换成视频）
    std::ofstream file(filename, std::ios::binary);
    file << "P6\n"
         << frameWidth << " " << frameHeight << "\n255\n";
    file.write((char *)flipped.data(), flipped.size());
    file.close();
}

bool initOpenGL()
{
    // 初始化GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // 隐藏窗口，因为我们只是渲染视频

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Skinning Animation", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);

    return true;
}

void setupMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);

    // 位置
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    // 法线
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    // 骨骼ID
    glVertexAttribIPointer(2, 4, GL_INT, sizeof(Vertex), (void *)offsetof(Vertex, boneIDs));
    glEnableVertexAttribArray(2);

    // 权重
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, weights));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}

void render()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.use();

    // 设置变换矩阵
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(15, 5, 0), // 相机位置
        glm::vec3(0, 4, 0),  // 看向中心
        glm::vec3(0, 1, 0)   // 上方向
    );
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.0f);

    shader.setMat4("uModel", model);
    shader.setMat4("uView", view);
    shader.setMat4("uProjection", projection);

    // 计算骨骼变换矩阵（从rest pose到当前pose）
    std::vector<glm::mat4> boneMatrices(skeleton.bones.size());

    for (size_t i = 0; i < skeleton.bones.size(); ++i)
    {
        int p = skeleton.bones[i].parent;

        if (p == -1)
        {
            // 根骨直接使用局部 pose * invRest
            // spine 固定
            boneMatrices[i] = skeleton.bones[i].poseMatrix * skeleton.bones[i].invRestMatrix;
        }
        else
        {
            // 父骨是 spine/root 时，累乘父骨，但 spine 不随腿摆动
            boneMatrices[i] = boneMatrices[p] * skeleton.bones[i].poseMatrix * skeleton.bones[i].invRestMatrix;
        }
    }

    shader.setMat4Array("uBoneMatrices", boneMatrices);

    // 光照
    shader.setVec3("uLightDir", glm::vec3(0.5f, -1.0f, 0.3f));
    shader.setVec3("uLightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    shader.setVec3("uViewPos", glm::vec3(0, 5, 15));

    // 渲染网格
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glfwSwapBuffers(window);
}

int main()
{
    // 1. 读取网格
    std::cout << "Loading mesh..." << std::endl;
    if (!mesh.loadOBJ("assets/skeleton.obj"))
    {
        std::cerr << "Failed to load mesh file" << std::endl;
        return -1;
    }

    // 2. 读取骨架
    std::cout << "Loading skeleton..." << std::endl;
    if (!skeleton.loadFromJSON("assets/skeleton.json"))
    {
        std::cerr << "Failed to load skeleton file" << std::endl;
        return -1;
    }

    for (size_t i = 0; i < skeleton.bones.size(); i++)
    {
        std::cout << i << " : " << skeleton.bones[i].name << std::endl;
    }

    // 3. 计算热传导权重
    std::cout << "Computing heat diffusion weithts..." << std::endl;
    HeatSkinning::computeWeights(mesh, skeleton);

    // 4. 初始化OpenGL
    std::cout << "Initializing OpenGL..." << std::endl;
    if (!initOpenGL())
    {
        return -1;
    }

    // 加载shader
    if (!shader.loadFromFiles("shaders/skinning.vert", "shaders/skinning.frag"))
    {
        std::cerr << "Failed to load shader" << std::endl;
        return -1;
    }

    setupMesh();

    // 5. 渲染视频帧
    std::cout << "Start render " << TOTAL_FRAMES << " frame..." << std::endl;

// 创建output目录
#ifdef _WIN32
    system("if not exist output mkdir output");
#else
    system("mkdir -p output");
#endif

    for (int frame = 0; frame < TOTAL_FRAMES; frame++)
    {
        float time = (float)frame / FPS;

        // 调试：打印前几帧的帧号和时间
        if (frame < 5 || frame == 150 || frame == 300)
        {
            std::cout << "\n=== Frame " << frame << " === time=" << time << std::endl;
        }

        // 更新动画
        updateWalkingAnimation(time, skeleton);

        // 调试：检查关键骨骼的变换矩阵是否变化
        if (frame < 3 || frame == 150 || frame == 300)
        {
            for (size_t i = 0; i < skeleton.bones.size(); i++)
            {
                const std::string &name = skeleton.bones[i].name;
                if (name.find("thigh.L") != std::string::npos)
                {
                    glm::mat4 boneTransform = skeleton.bones[i].poseMatrix * skeleton.bones[i].invRestMatrix;
                    glm::vec3 transformPos = glm::vec3(boneTransform[3]);
                    std::cout << "[DEBUG Render Frame " << frame << "] " << name
                              << " boneTransform pos=(" << transformPos.x << ","
                              << transformPos.y << "," << transformPos.z << ")" << std::endl;
                    break;
                }
            }
        }

        // 渲染
        render();
        glFinish();

        // 保存帧
        std::ostringstream filename;
        filename << "output/frame_" << std::setfill('0') << std::setw(5) << frame << ".ppm";
        saveFrame(filename.str(), WINDOW_WIDTH, WINDOW_HEIGHT);

        if ((frame + 1) % 30 == 0)
        {
            std::cout << (frame + 1) << " /" << TOTAL_FRAMES << " frames has been rendered." << std::endl;
        }

        glfwPollEvents();
    }

    std::cout << "Rendering completed! Frames saved to output/ directory" << std::endl;
    std::cout << "Use the following command to convert frames to video:" << std::endl;
    std::cout << "ffmpeg -r 30 -i output/frame_%05d.ppm -c:v libx264 -pix_fmt yuv420p output/animation.mp4" << std::endl;

    glfwTerminate();
    return 0;
}
