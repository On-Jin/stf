#include "Model.hpp"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

Model::Model()
    : positionMax_(0.f),
      positionMin_(0.f),
      positionCenter_(0.f),
      interScaling_(1.f) {
}

Model::Model(std::string const &path)
    : positionMax_(0.f),
      positionMin_(0.f),
      positionCenter_(0.f),
      interScaling_(1.f) {
    setModel(path);
}

void Model::setModel(std::string const &path) {
    clean_();
    path_ = path;
    loadModel_();
    setupScaling_();
}

Model::~Model() {
    clean_();
}

void Model::clean_() {
    mesh_.clear();
}

std::deque<Mesh> const &Model::getMeshes() const {
    return (mesh_);
}

float Model::getInterScaling() const {
    return (interScaling_);
}
glm::vec3 Model::getPositionCenterRelativeToOrigin() const {
    return (positionCenter_);
}

void Model::render(GLenum typeOfDraw) const {
    for (const auto &i: mesh_)
        i.render(typeOfDraw);
}
void Model::render(Shader &shader, GLenum typeOfDraw) const {
    for (const auto &i: mesh_)
        i.render(shader, typeOfDraw);
}

void Model::loadModel_() {
    Assimp::Importer import;
    const aiScene *scene = import.ReadFile(path_.generic_string(), aiProcess_Triangulate | aiProcess_GenUVCoords | aiProcess_GenNormals | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw(Model::ModelException(std::string("ERROR::ASSIMP::") + import.GetErrorString()));
    }
    std::cout << "[path_] : " << path_ << std::endl;
    directory_ = path_.parent_path();
    std::cout << "[directory_] : " << directory_ << std::endl;
    processNode_(scene->mRootNode, scene);
}

void Model::processNode_(aiNode *node, const aiScene *scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh_(mesh, scene);
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode_(node->mChildren[i], scene);
    }
}

void Model::makeSphere(float radius, float sectorCount, float stackCount) {
    clean_();

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    float x, y, z, xy;                           // vertex position
    float nx, ny, nz, lengthInv = 1.0f / radius; // vertex normal
    float s, t;                                  // vertex texCoord

    float sectorStep = 2 * 3.14159265f / sectorCount;
    float stackStep = 3.14159265f / stackCount;
    float sectorAngle, stackAngle;

    for (int i = 0; i <= stackCount; ++i) {
        stackAngle = 3.14159265f / 2 - i * stackStep; // starting from pi/2 to -pi/2
        xy = radius * cosf(stackAngle);               // r * cos(u)
        z = radius * sinf(stackAngle);                // r * sin(u)

        // add (sectorCount+1) vertices per stack
        // the first and last vertices have same position and normal, but different tex coords
        for (int j = 0; j <= sectorCount; ++j) {
            Vertex vertex;

            sectorAngle = j * sectorStep; // starting from 0 to 2pi

            // vertex position (x, y, z)
            x = xy * cosf(sectorAngle); // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle); // r * cos(u) * sin(v)
            vertex.position = glm::vec3(x, y, z);

            // normalized vertex normal (nx, ny, nz)
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            vertex.normal = glm::vec3(nx, ny, nz);

            // vertex tex coord (s, t) range between [0, 1]
            s = (float)j / sectorCount;
            t = (float)i / stackCount;
            vertex.uv = glm::vec2(s, t);

            if (vertex.position.x > positionMax_.x)
                positionMax_.x = vertex.position.x;
            if (vertex.position.y > positionMax_.y)
                positionMax_.y = vertex.position.y;
            if (vertex.position.z > positionMax_.z)
                positionMax_.z = vertex.position.z;

            if (vertex.position.x < positionMin_.x)
                positionMin_.x = vertex.position.x;
            if (vertex.position.y < positionMin_.y)
                positionMin_.y = vertex.position.y;
            if (vertex.position.z < positionMin_.z)
                positionMin_.z = vertex.position.z;

            vertices.push_back(vertex);
        }
    }

    int k1, k2;
    for (int i = 0; i < stackCount; ++i) {
        k1 = i * (sectorCount + 1); // beginning of current stack
        k2 = k1 + sectorCount + 1;  // beginning of next stack

        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            // 2 triangles per sector excluding first and last stacks
            // k1 => k2 => k1+1
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            // k1+1 => k2 => k2+1
            if (i != (stackCount - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    mesh_.emplace_back(vertices, indices);
    setupScaling_();
}

void Model::processMesh_(aiMesh *mesh, const aiScene *scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    std::cout << "node NumMeshes : " << mesh->mNumVertices << std::endl;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        if (mesh->mTextureCoords[0]) // Each 1st column = a texture (Jusqu'a 8)
            vertex.uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        else
            vertex.uv = glm::vec2(0.0f, 0.0f);
        vertices.push_back(vertex);

        if (vertex.position.x > positionMax_.x)
            positionMax_.x = vertex.position.x;
        if (vertex.position.y > positionMax_.y)
            positionMax_.y = vertex.position.y;
        if (vertex.position.z > positionMax_.z)
            positionMax_.z = vertex.position.z;

        if (vertex.position.x < positionMin_.x)
            positionMin_.x = vertex.position.x;
        if (vertex.position.y < positionMin_.y)
            positionMin_.y = vertex.position.y;
        if (vertex.position.z < positionMin_.z)
            positionMin_.z = vertex.position.z;
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) //Always 3 : I make Triangular Flag Option
            indices.push_back(face.mIndices[j]);
    }

    // Traitement des matériaux
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

    std::vector<Texture> diffuseMaps = loadMaterialTextures_(material, aiTextureType_DIFFUSE, Texture::eType::DIFFUSE);
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

    std::vector<Texture> specularMaps = loadMaterialTextures_(material, aiTextureType_SPECULAR, Texture::eType::SPECULAR);
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    mesh_.emplace_back(vertices, indices, textures);
}

void Model::setupScaling_() {
    glm::vec3 diff;
    float scaling;

    diff = glm::abs(positionMax_ - positionMin_);
    if (diff.x > diff.y && diff.x > diff.z)
        scaling = 1.f / diff.x;
    else if (diff.y > diff.x && diff.y > diff.z)
        scaling = 1.f / diff.y;
    else
        scaling = 1.f / diff.z;
    diff = positionMax_ - positionMin_;
    interScaling_ = scaling;
    diff = diff * 0.5f + positionMin_;
    positionCenter_ = diff;
}

std::vector<Texture> Model::loadMaterialTextures_(aiMaterial *mat, aiTextureType type, Texture::eType eType) {
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);

        Texture texture;
        texture.id = Texture::TextureFromFile(str.C_Str(), directory_);
        texture.type = eType;
        texture.path = std::string(str.C_Str());
        textures.push_back(texture);
    }
    return (textures);
}

bool Model::debug_ = true;

Model::ModelException::~ModelException() noexcept = default;

Model::ModelException::ModelException() noexcept
    : error_("Error on Glfw constructor") {
}

Model::ModelException::ModelException(
    std::string s) noexcept
    : error_(s) {
}

Model::ModelException::ModelException(
    Model::ModelException const &src) noexcept
    : error_(src.error_) {
    error_ = src.error_;
}

const char *
Model::ModelException::what() const noexcept {
    return (error_.c_str());
}
