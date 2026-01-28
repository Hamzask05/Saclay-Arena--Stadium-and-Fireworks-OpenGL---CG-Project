#include <filesystem>
namespace fs = std::filesystem;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <deque>
#include <cmath>
#include <algorithm>

// ============================================================
// CAMERA
// ============================================================
glm::vec3 cameraPos   = glm::vec3(0.0f, 15.0f, 90.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

float yaw = -90.0f; float pitch = -10.0f;
float lastX = 640, lastY = 360;
bool firstMouse = true;
float speed = 50.0f;
float sensitivity = 0.1f;

// ============================================================
// STRUCTS
// ============================================================
struct Mesh {
    GLuint VAO;
    int vertexCount;
    unsigned int textureID;
    glm::vec3 color;
    bool hasTexture;
};

struct Particle {
    glm::vec3 Position;
    glm::vec3 Velocity;
    glm::vec4 Color;
    glm::vec4 BaseColor;
    float Life;
    float MaxLife;
    float Size;
    int Type;
};

struct FireworkLight {
    glm::vec3 Position;
    glm::vec3 Color;
    float Intensity;
};

// ============================================================
// GLOBALS
// ============================================================
const int MAX_PARTICLES = 30000;
std::vector<Particle> particles;
std::deque<FireworkLight> activeLights;
std::vector<glm::vec3> pendingExplosions;

unsigned int particleVAO, particleVBO;
float fireworkTimer = 0.0f;

// PAUSE & FULLSCREEN GLOBALS
bool isPaused = false;
bool lastSpaceState = false;
float simulationTime = 0.0f;

// Fullscreen State
bool isFullscreen = false;
bool lastFState = false;
int savedWinX, savedWinY, savedWinW, savedWinH;

// 1-3-2-1 FORMATION (Only Strikers used now)
glm::vec3 startPositions[7] = {
    glm::vec3(48.0f, 0.0f, 0.0f),
    glm::vec3(35.0f, 0.0f, -15.0f),
    glm::vec3(35.0f, 0.0f, 0.0f),
    glm::vec3(35.0f, 0.0f, 15.0f),
    glm::vec3(20.0f, 0.0f, -10.0f),
    glm::vec3(20.0f, 0.0f, 10.0f),
    glm::vec3(12.0f, 0.0f, 0.0f)    // 6: STRIKER (Passer)
};

glm::vec3 currentPosRed[7];
glm::vec3 currentPosGreen[7];
float mascotOffsets[7];
int mascotAnims[7];

// BALL DATA
glm::vec3 ballPos = glm::vec3(0.0f, 2.5f, 0.0f);
float ballRotationAngle = 0.0f;
std::vector<Mesh> ballMeshes;

// BALL LOGIC STATE
int ballState = 0;
float passProgress = 0.0f;
float holdTimer = 0.0f;

// FLOODLIGHTS (Positions Only - Models are hidden)
glm::vec3 floodLights[2] = {
    glm::vec3(0.0f, 80.0f, 60.0f),  // Center Right (High up)
    glm::vec3(0.0f, 80.0f, -60.0f)  // Center Left (High up)
};

// ============================================================
// HELPER FUNCTIONS
// ============================================================

void spawnExplosion(glm::vec3 origin) {
    if (particles.size() > MAX_PARTICLES - 1000) return;
    glm::vec3 colors[] = { glm::vec3(1,0.1,0.1), glm::vec3(0.1,1,0.1), glm::vec3(0.2,0.5,1), glm::vec3(1,0.8,0), glm::vec3(0,1,1), glm::vec3(1,0,1) };
    glm::vec3 baseColor = colors[rand()%6];
    FireworkLight light; light.Position = origin; light.Color = baseColor; light.Intensity = 5.0f;
    activeLights.push_back(light); if(activeLights.size() > 4) activeLights.pop_front();

    for (int i = 0; i < 800; i++) {
        Particle p; p.Position = origin;
        float theta = (float)(rand() % 360) * 0.0174533f;
        float phi = acos(1.0f - 2.0f * (float)(rand() % 1000) / 1000.0f);
        float speed = 15.0f + ((rand() % 100) / 10.0f);
        float vx = sin(phi) * cos(theta); float vy = sin(phi) * sin(theta); float vz = cos(phi);
        p.Velocity = glm::vec3(vx, vy, vz) * speed;
        p.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); p.BaseColor = glm::vec4(baseColor, 1.0f);
        p.Life = 1.5f + ((rand() % 100) / 100.0f); p.MaxLife = p.Life; p.Size = 4.0f + ((rand() % 100) / 50.0f);
        p.Type = 0; particles.push_back(p);
    }
}

void launchRocket() {
    if (particles.size() > MAX_PARTICLES - 100) return;
    float rx = ((rand()%120)-60)*1.0f; float rz = ((rand()%80)-40)*1.0f;
    Particle p; p.Position = glm::vec3(rx,0,rz); p.Velocity = glm::vec3(0,70,0);
    p.Color = glm::vec4(1,1,0.8,1); p.BaseColor = p.Color; p.Life = 0.8f; p.MaxLife = 0.8f; p.Size = 6.0f; p.Type = 1;
    particles.push_back(p);
}

unsigned int loadTextureFromFile(const char* filename, const std::string& directory, bool flip) {
    std::filesystem::path p(filename);
    std::string pureName = p.filename().string();
    std::string path1 = directory + "/" + pureName;
    std::string path2 = "../textures/" + pureName;
    std::string path3 = "textures/" + pureName;

    bool isLogo = false;
    if(pureName.find("pompom") != std::string::npos) pureName = "pompom.png";
    if(pureName.find("WhatsApp") != std::string::npos) pureName = "ad.jpg";
    if(pureName.find("logo") != std::string::npos || pureName.find("Logo") != std::string::npos) {
        pureName = "logo pos.png"; path2 = "../textures/logo pos.png"; path3 = "textures/logo pos.png"; isLogo = true;
    }

    unsigned int textureID;
    glGenTextures(1, &textureID);
    if(isLogo) stbi_set_flip_vertically_on_load(false); else stbi_set_flip_vertically_on_load(flip);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path1.c_str(), &width, &height, &nrComponents, 0);
    if(!data) data = stbi_load(path2.c_str(), &width, &height, &nrComponents, 0);
    if(!data) data = stbi_load(path3.c_str(), &width, &height, &nrComponents, 0);

    if (data) {
        GLenum format = (nrComponents == 4) ? GL_RGBA : GL_RGB;
        if (nrComponents == 1) format = GL_RED;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    } else {
        stbi_image_free(data);
        return 0;
    }
    return textureID;
}

bool stringContains(std::string haystack, std::string needle) {
    std::transform(haystack.begin(), haystack.end(), haystack.begin(), ::tolower);
    std::transform(needle.begin(), needle.end(), needle.begin(), ::tolower);
    return haystack.find(needle) != std::string::npos;
}

std::vector<Mesh> loadModel(std::string path, bool flipTextures) {
    std::vector<Mesh> meshes;
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_OptimizeMeshes);
    if (!scene || !scene->HasMeshes()) {
        std::cerr << "[CRITICAL] Failed: " << path << std::endl;
        return meshes;
    }
    
    std::string directory = path.substr(0, path.find_last_of('/'));
    std::vector<unsigned int> matTextures(scene->mNumMaterials, 0);
    std::vector<glm::vec3> matColors(scene->mNumMaterials, glm::vec3(0.5f));

    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* material = scene->mMaterials[i];
        aiString name; material->Get(AI_MATKEY_NAME, name);
        std::string matNameString = name.C_Str();

        bool isChair = false;
        if(stringContains(matNameString, "chair") || stringContains(matNameString, "seat") ||
           stringContains(matNameString, "siege") || stringContains(matNameString, "gradin") ||
           stringContains(matNameString, "bleacher")) { isChair = true; }

        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString str; material->GetTexture(aiTextureType_DIFFUSE, 0, &str);
            std::string texPath = str.C_Str();
            if(stringContains(texPath, "chair") || stringContains(texPath, "seat")) isChair = true;
            if(!isChair) matTextures[i] = loadTextureFromFile(str.C_Str(), directory, flipTextures);
        }

        if(isChair) {
            matTextures[i] = 0;
            matColors[i] = glm::vec3(0.54f, 0.81f, 0.94f);
        } else {
            aiColor3D color(0.5f, 0.5f, 0.5f);
            material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            matColors[i] = glm::vec3(color.r, color.g, color.b);
        }
    }
    
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        if (!(mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE)) continue;
        std::vector<float> vertices; vertices.reserve(mesh->mNumFaces * 3 * 8);
        for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
            aiFace face = mesh->mFaces[f];
            for (unsigned int k = 0; k < face.mNumIndices; k++) {
                unsigned int index = face.mIndices[k];
                aiVector3D pos = mesh->mVertices[index];
                vertices.push_back(pos.x); vertices.push_back(pos.y); vertices.push_back(pos.z);
                if (mesh->HasNormals()) {
                    aiVector3D n = mesh->mNormals[index];
                    vertices.push_back(n.x); vertices.push_back(n.y); vertices.push_back(n.z);
                } else { vertices.push_back(0); vertices.push_back(1); vertices.push_back(0); }
                if (mesh->mTextureCoords[0]) {
                    aiVector3D uv = mesh->mTextureCoords[0][index];
                    vertices.push_back(uv.x); vertices.push_back(uv.y);
                } else { vertices.push_back(0); vertices.push_back(0); }
            }
        }
        Mesh newMesh;
        glGenVertexArrays(1, &newMesh.VAO); GLuint VBO; glGenBuffers(1, &VBO);
        glBindVertexArray(newMesh.VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float))); glEnableVertexAttribArray(2);
        newMesh.vertexCount = vertices.size() / 8;
        unsigned int matIdx = mesh->mMaterialIndex;
        newMesh.textureID = matTextures[matIdx];
        newMesh.hasTexture = (newMesh.textureID != 0);
        newMesh.color = matColors[matIdx];
        meshes.push_back(newMesh);
    }
    return meshes;
}

unsigned int loadCubemap(std::vector<std::string> faces) {
    unsigned int textureID; glGenTextures(1, &textureID); return textureID;
}

// On a mis les SHADERS car on a un bug qui nous empeche d'importer tous les fichiers sur github ( on depasse la limite de memoire autorisee // a cause du stade et sa taille
const char* vertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
out vec3 FragPos; out vec3 Normal; out vec2 TexCoord;
uniform mat4 MVP; uniform mat4 model;
uniform int animType; uniform float time; uniform float randomOffset; 
void main() {
    vec3 pos = aPos;
    if (animType == 0) { // Idle
        pos.y += sin(time * 3.0 + randomOffset) * 0.02 * pos.y;
    } else if (animType == 2) { // Running
        float run = sin(time * 12.0 + randomOffset);
        pos.y += abs(run) * 0.8; pos.x += cos(time * 12.0) * 0.2; pos.z += pos.y * 0.3; 
    } 
    FragPos = vec3(model * vec4(pos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = MVP * vec4(pos, 1.0);
}
)";

const char* fragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;
in vec3 FragPos; in vec3 Normal; in vec2 TexCoord;
uniform sampler2D texture1;
uniform vec3 lightPos; uniform vec3 viewPos; uniform vec3 lightColor; 
uniform vec3 teamColor; uniform bool useTeamColor; 
uniform vec3 firePos[4]; uniform vec3 fireColor[4]; uniform float fireIntensity[4];
uniform vec3 floodPos[2]; // 2 FLOODLIGHTS
uniform bool hasTexture; uniform vec3 meshColor;
uniform bool isEmissive; 

void main() {
    if(isEmissive) {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0); 
        return;
    }

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // 1. Ambient (INCREASED to 0.6 for better visibility)
    vec3 ambient = 0.6 * lightColor;
    vec3 lighting = ambient;

    // 2. Floodlights (2 Lights) - Increased Intensity & Slight Specular
    for(int i=0; i<2; i++) {
        vec3 lightDir = normalize(floodPos[i] - FragPos);
        float dist = length(floodPos[i] - FragPos);
        float att = 1.0 / (1.0 + 0.0001 * dist + 0.00005 * dist * dist);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, norm);  
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
        
        // Intensity 0.6 (Medium) + Specular 0.2 (Slight sheen)
        lighting += (diff + spec * 0.2) * vec3(1.0, 1.0, 0.9) * att * 0.6; 
    }

    // 3. Fireworks Lights (Brighter)
    for(int i=0; i<4; i++) {
        if(fireIntensity[i] > 0.01) {
            vec3 fDir = normalize(firePos[i] - FragPos);
            float dist = length(firePos[i] - FragPos);
            float att = 1.0 / (1.0 + 0.01 * dist + 0.0005 * dist * dist);
            float fDiff = max(dot(norm, fDir), 0.0);
            // Intensity multiplier 0.8
            lighting += fDiff * fireColor[i] * fireIntensity[i] * att * 0.8; 
        }
    }
    
    vec4 objectColor;
    if(hasTexture) objectColor = texture(texture1, TexCoord); else objectColor = vec4(meshColor, 1.0);
    if (useTeamColor) objectColor = mix(objectColor, vec4(teamColor, 1.0), 0.6); 
    
    FragColor = vec4(lighting * objectColor.rgb, objectColor.a);
}
)";

const char* particleVertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in float aSize; 
out vec4 ParticleColor;
uniform mat4 MVP;
void main() {
    ParticleColor = aColor;
    gl_Position = MVP * vec4(aPos, 1.0);
    float dist = gl_Position.w;
    gl_PointSize = (400.0 * aSize) / dist; 
    if(gl_PointSize < 2.0) gl_PointSize = 2.0;
}
)";
const char* particleFragmentSrc = R"(
#version 330 core
out vec4 FragColor; in vec4 ParticleColor;
void main() { 
    vec2 c = gl_PointCoord - vec2(0.5);
    float a = 1.0 - length(c) * 2.0;
    if(a < 0.0) discard;
    FragColor = vec4(ParticleColor.rgb, ParticleColor.a * a);
}
)";

// le MAIN commence ici réellement
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = (xpos - lastX) * sensitivity; float yoffset = (lastY - ypos) * sensitivity;
    lastX = xpos; lastY = ypos; yaw += xoffset; pitch += yoffset;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void processInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    float v = speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) v *= 2.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += v * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= v * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * v;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * v;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!lastSpaceState) { isPaused = !isPaused; lastSpaceState = true; }
    } else lastSpaceState = false;

    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (!lastFState) {
            lastFState = true; isFullscreen = !isFullscreen;
            if (isFullscreen) {
                glfwGetWindowPos(window, &savedWinX, &savedWinY); glfwGetWindowSize(window, &savedWinW, &savedWinH);
                GLFWmonitor* m = glfwGetPrimaryMonitor(); const GLFWvidmode* v = glfwGetVideoMode(m);
                glfwSetWindowMonitor(window, m, 0, 0, v->width, v->height, v->refreshRate);
            } else glfwSetWindowMonitor(window, nullptr, savedWinX, savedWinY, savedWinW, savedWinH, 0);
        }
    } else lastFState = false;
}

GLuint createProgram(const char* vSrc, const char* fSrc) {
    GLuint v = glCreateShader(GL_VERTEX_SHADER); glShaderSource(v, 1, &vSrc, nullptr); glCompileShader(v);
    GLuint f = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(f, 1, &fSrc, nullptr); glCompileShader(f);
    GLuint p = glCreateProgram(); glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    return p;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Ultimate Football", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    glewInit();
    glEnable(GL_DEPTH_TEST);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // INIT PLAYERS
    for(int i=0; i<7; i++) {
        currentPosRed[i] = startPositions[i];
        glm::vec3 m = startPositions[i]; m.x = -m.x; currentPosGreen[i] = m;
        mascotOffsets[i] = (float)(rand()%100)/10.0f;
        int r = rand()%10; if(r<4) mascotAnims[i]=0; else if(r<8) mascotAnims[i]=2; else mascotAnims[i]=1;
        if(i == 6) mascotAnims[i] = 2;
    }

    GLuint mainShader = createProgram(vertexShaderSrc, fragmentShaderSrc);
    GLuint particleShader = createProgram(particleVertexSrc, particleFragmentSrc);

    std::vector<Mesh> stadiumMeshes = loadModel("../models/stadium/terrainFinal.obj", false);
    std::vector<Mesh> mascot1Meshes  = loadModel("../models/mascot/mascot1.obj", true);
    std::vector<Mesh> mascot2Meshes  = loadModel("../models/mascot/mascot2.obj", true);
    ballMeshes = loadModel("../models/balle/balleFoot.obj", true);

    glGenVertexArrays(1, &particleVAO); glGenBuffers(1, &particleVBO);
    glBindVertexArray(particleVAO); glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8*MAX_PARTICLES, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(7*sizeof(float))); glEnableVertexAttribArray(2);

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window, deltaTime);

        if (!isPaused) {
            simulationTime += deltaTime;

            // Mascots
            float passSpeed = 2.0f;
            float t = (sin(simulationTime * passSpeed) + 1.0f) / 2.0f;
            glm::vec3 targetRed = startPositions[6]; targetRed.x -= 2.0f; currentPosRed[6] = targetRed;
            glm::vec3 targetGreen = startPositions[6]; targetGreen.x = -targetGreen.x; targetGreen.x += 2.0f; currentPosGreen[6] = targetGreen;
            currentPosRed[6].z += sin(simulationTime * 10.0f) * 0.2f;
            currentPosGreen[6].z += sin(simulationTime * 10.0f + 1.0f) * 0.2f;

            // Ball Logic (Linear)
            glm::vec3 p1 = currentPosRed[6]; p1.x -= 3.5f;
            glm::vec3 p2 = currentPosGreen[6]; p2.x += 3.5f;
            if (ballState == 0) { ballPos = p1; holdTimer += deltaTime; if (holdTimer > 1.0f) { holdTimer = 0.0f; ballState = 1; passProgress = 0.0f; } }
            else if (ballState == 1) { passProgress += deltaTime * 2.0f; if (passProgress >= 1.0f) { passProgress = 1.0f; ballState = 2; } ballPos = glm::mix(p1, p2, passProgress); ballRotationAngle -= deltaTime * 800.0f; }
            else if (ballState == 2) { ballPos = p2; holdTimer += deltaTime; if (holdTimer > 1.0f) { holdTimer = 0.0f; ballState = 3; passProgress = 0.0f; } }
            else if (ballState == 3) { passProgress += deltaTime * 2.0f; if (passProgress >= 1.0f) { passProgress = 1.0f; ballState = 0; } ballPos = glm::mix(p2, p1, passProgress); ballRotationAngle += deltaTime * 800.0f; }
            ballPos.y = 2.5f;

            // Fireworks
            fireworkTimer += deltaTime;
            if (fireworkTimer > 0.8f) { launchRocket(); fireworkTimer = 0.0f; }
            
            std::vector<float> pData;
            for (int i = 0; i < particles.size(); i++) {
                Particle& p = particles[i]; p.Life -= deltaTime;
                if (p.Life > 0.0f) {
                    if (p.Type == 1) { p.Position += p.Velocity * deltaTime; p.Velocity.y -= 9.8f * deltaTime;
                        Particle tr; tr.Position = p.Position; tr.Velocity = glm::vec3(0); tr.Color = glm::vec4(1.0f, 1.0f, 0.8f, 1.0f); tr.Life = 0.2f; tr.MaxLife = 0.2f; tr.Size = 6.0f; tr.Type = 2; particles.push_back(tr);
                    } else if (p.Type == 0) { p.Velocity *= 0.95f; p.Velocity.y -= 5.0f * deltaTime; p.Position += p.Velocity * deltaTime;
                        float r = p.Life / p.MaxLife; p.Color = glm::mix(p.BaseColor, glm::vec4(1.0f), r * 0.3f); p.Color.a = r; }
                    else if (p.Type == 2) { p.Color.a = p.Life / p.MaxLife; }
                    pData.push_back(p.Position.x); pData.push_back(p.Position.y); pData.push_back(p.Position.z);
                    pData.push_back(p.Color.r); pData.push_back(p.Color.g); pData.push_back(p.Color.b); pData.push_back(p.Color.a);
                    pData.push_back(p.Size);
                } else { if (p.Type == 1) pendingExplosions.push_back(p.Position); particles.erase(particles.begin() + i); i--; }
            }
            for(auto& pos : pendingExplosions) spawnExplosion(pos);
            pendingExplosions.clear();
            if (!pData.empty()) { glBindVertexArray(particleVAO); glBindBuffer(GL_ARRAY_BUFFER, particleVBO); glBufferSubData(GL_ARRAY_BUFFER, 0, pData.size() * sizeof(float), pData.data()); }
        }

        // rla partie suivante est le rendu
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int winW, winH; glfwGetFramebufferSize(window, &winW, &winH);
        float aspect = (float)winW / (float)winH;
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 2000.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos+cameraFront, cameraUp);
        glm::mat4 MVP;

        glUseProgram(mainShader);
        glUniform3f(glGetUniformLocation(mainShader, "lightPos"), 50.0f, 200.0f, 100.0f);
        glUniform3f(glGetUniformLocation(mainShader, "lightColor"), 0.5f, 0.5f, 0.6f);
        glUniform3fv(glGetUniformLocation(mainShader, "viewPos"), 1, &cameraPos[0]);
        
        // Pass Floodlight Data (2 Lights)
        for(int i=0; i<2; i++) {
            std::string name = "floodPos[" + std::to_string(i) + "]";
            glUniform3fv(glGetUniformLocation(mainShader, name.c_str()), 1, &floodLights[i][0]);
        }

        // Pass Firework Light Data
        for(int i=0; i<4; i++) {
            if(i < activeLights.size()) {
                std::string base = "firePos[" + std::to_string(i) + "]";
                std::string cbase = "fireColor[" + std::to_string(i) + "]";
                std::string ibase = "fireIntensity[" + std::to_string(i) + "]";
                glUniform3fv(glGetUniformLocation(mainShader, base.c_str()), 1, &activeLights[i].Position[0]);
                glUniform3fv(glGetUniformLocation(mainShader, cbase.c_str()), 1, &activeLights[i].Color[0]);
                glUniform1f(glGetUniformLocation(mainShader, ibase.c_str()), activeLights[i].Intensity);
                if(!isPaused) activeLights[i].Intensity -= deltaTime * 1.5f;
            }
        }
        if(!activeLights.empty() && activeLights.front().Intensity <= 0.0f) activeLights.pop_front();

        glUniform1i(glGetUniformLocation(mainShader, "useTeamColor"), 0);
        glUniform1i(glGetUniformLocation(mainShader, "animType"), -1);
        glUniform1i(glGetUniformLocation(mainShader, "isEmissive"), 0);
        
        // Draw Stadium
        glm::mat4 modelStadium = glm::mat4(1.0f);
        MVP = projection * view * modelStadium;
        glUniformMatrix4fv(glGetUniformLocation(mainShader, "MVP"), 1, GL_FALSE, glm::value_ptr(MVP));
        glUniformMatrix4fv(glGetUniformLocation(mainShader, "model"), 1, GL_FALSE, glm::value_ptr(modelStadium));
        for (auto& mesh : stadiumMeshes) {
            glUniform1i(glGetUniformLocation(mainShader, "hasTexture"), mesh.hasTexture);
            if (mesh.hasTexture) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, mesh.textureID); }
            else glUniform3fv(glGetUniformLocation(mainShader, "meshColor"), 1, &mesh.color[0]);
            glBindVertexArray(mesh.VAO); glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
        }

        // Draw Ball
        glUniform1i(glGetUniformLocation(mainShader, "useTeamColor"), 0);
        glm::mat4 modelBall = glm::translate(glm::mat4(1.0f), ballPos);
        modelBall = glm::rotate(modelBall, glm::radians(ballRotationAngle), glm::vec3(0,0,1));
        modelBall = glm::scale(modelBall, glm::vec3(0.8f));
        MVP = projection * view * modelBall;
        glUniformMatrix4fv(glGetUniformLocation(mainShader, "MVP"), 1, GL_FALSE, glm::value_ptr(MVP));
        glUniformMatrix4fv(glGetUniformLocation(mainShader, "model"), 1, GL_FALSE, glm::value_ptr(modelBall));
        for (auto& mesh : ballMeshes) {
            glUniform1i(glGetUniformLocation(mainShader, "hasTexture"), mesh.hasTexture);
            if(mesh.hasTexture) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, mesh.textureID); }
            else glUniform3fv(glGetUniformLocation(mainShader, "meshColor"), 1, &mesh.color[0]);
            glBindVertexArray(mesh.VAO); glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
        }

        glUniform1f(glGetUniformLocation(mainShader, "time"), simulationTime);

        // Draw Strikers
        for(int i = 6; i < 7; i++) {
            glUniform1f(glGetUniformLocation(mainShader, "randomOffset"), mascotOffsets[i]);
            glUniform1i(glGetUniformLocation(mainShader, "animType"), mascotAnims[i]);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), currentPosRed[i]);
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0,1,0));
            model = glm::rotate(model, glm::radians(mascotOffsets[i] * 5.0f), glm::vec3(0,1,0));
            model = glm::scale(model, glm::vec3(0.8f));
            MVP = projection * view * model;
            glUniformMatrix4fv(glGetUniformLocation(mainShader, "MVP"), 1, GL_FALSE, glm::value_ptr(MVP));
            glUniformMatrix4fv(glGetUniformLocation(mainShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
            for (auto& mesh : mascot1Meshes) {
                glUniform1i(glGetUniformLocation(mainShader, "hasTexture"), mesh.hasTexture);
                if(mesh.hasTexture) glBindTexture(GL_TEXTURE_2D, mesh.textureID);
                else glUniform3fv(glGetUniformLocation(mainShader, "meshColor"), 1, &mesh.color[0]);
                glBindVertexArray(mesh.VAO); glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
            }
        }
        for(int i = 6; i < 7; i++) {
            glUniform1f(glGetUniformLocation(mainShader, "randomOffset"), mascotOffsets[i] + 10.0f);
            glUniform1i(glGetUniformLocation(mainShader, "animType"), mascotAnims[i]);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), currentPosGreen[i]);
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0,1,0));
            model = glm::rotate(model, glm::radians(mascotOffsets[i] * 5.0f), glm::vec3(0,1,0));
            model = glm::scale(model, glm::vec3(0.8f));
            MVP = projection * view * model;
            glUniformMatrix4fv(glGetUniformLocation(mainShader, "MVP"), 1, GL_FALSE, glm::value_ptr(MVP));
            glUniformMatrix4fv(glGetUniformLocation(mainShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
            for (auto& mesh : mascot2Meshes) {
                glUniform1i(glGetUniformLocation(mainShader, "hasTexture"), mesh.hasTexture);
                if(mesh.hasTexture) glBindTexture(GL_TEXTURE_2D, mesh.textureID);
                else glUniform3fv(glGetUniformLocation(mainShader, "meshColor"), 1, &mesh.color[0]);
                glBindVertexArray(mesh.VAO); glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
            }
        }

        // Draw Fireworks
        if (particles.size() > 0) {
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE); glEnable(GL_PROGRAM_POINT_SIZE);
            glUseProgram(particleShader);
            MVP = projection * view * glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(particleShader, "MVP"), 1, GL_FALSE, glm::value_ptr(MVP));
            glBindVertexArray(particleVAO);
            glDrawArrays(GL_POINTS, 0, particles.size());
            glDepthMask(GL_TRUE); glDisable(GL_BLEND);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
