#ifndef MESH_H
#define MESH_H

#include <string>
#include <vector>
#include <stb_image.h>

#include "../../vendor/glad/include/glad.h" // holds all OpenGL type declarations

#include "../../vendor/glm/glm.hpp"
#include "../../vendor/glm/gtc/matrix_transform.hpp"
#include "../../vendor/glm/gtc/quaternion.hpp"
#include "../../vendor/glm/gtc/type_ptr.hpp"

#include "Shader.h"

using namespace std;
using namespace glm;

struct Vertex
{
    // Vertex Position
    vec3 Position;
    // Vertex Normal
    vec3 Normal;
    // Vertex Tangent
    vec3 Tangent;
    // Texture Coordinates
    vec2 TexCoord;
};

enum class TextureType
{
    None,
    BaseColor,
    MetallicRoughness,
    Emissive,
    Normal
};

struct Texture
{
    unsigned int ID;
    GLuint slot;
    TextureType type;
    string path;

    Texture()
    {
        ID = 0;
        slot = 0;
        type = TextureType::None;
        path = std::string();
    }

    Texture(const char* image, TextureType texType, GLuint slot)
    {
        // Stores the width, height, and the number of color channels of the image
        int widthImg, heightImg, numColCh;
        // Flips the image so it appears right side up
        stbi_set_flip_vertically_on_load(true);
        // Reads the image from a file and stores it in bytes
        unsigned char* bytes = stbi_load(image, &widthImg, &heightImg, &numColCh, 0);

        // Generates an OpenGL texture object
        glGenTextures(1, &ID);
        // Assigns the texture to a Texture Unit
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, ID);

        // Configures the type of algorithm that is used to make the image smaller or bigger
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Configures the way the texture repeats (if it does at all)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Check what type of color channels the texture has and load it accordingly
        if (numColCh == 4)
            glTexImage2D (GL_TEXTURE_2D, 0, texType == TextureType::BaseColor ? GL_SRGB_ALPHA : GL_RGBA, widthImg, heightImg, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
        else if (numColCh == 3)
            glTexImage2D (GL_TEXTURE_2D, 0, texType == TextureType::BaseColor ? GL_SRGB_ALPHA : GL_RGBA, widthImg, heightImg, 0, GL_RGB, GL_UNSIGNED_BYTE, bytes);
        else if (numColCh == 1)
            glTexImage2D (GL_TEXTURE_2D, 0, texType == TextureType::BaseColor ? GL_SRGB_ALPHA : GL_RGBA, widthImg, heightImg, 0, GL_RED, GL_UNSIGNED_BYTE, bytes);
        else
            throw std::invalid_argument("Automatic Texture type recognition failed");

        // Generates MipMaps
        glGenerateMipmap(GL_TEXTURE_2D);

        // Deletes the image data as it is already in the OpenGL Texture object
        stbi_image_free(bytes);

        // Assigns the type of the texture ot the texture object
        type = texType;
        this->slot = slot;
        path = image;

        // Unbinds the OpenGL Texture object so that it can't accidentally be modified
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

struct Material_GLTF
{
    float metallicFactor;
    float roughnessFactor;

    Texture baseColorTexture;
    Texture metallicRoughnessTexture;
    Texture emissiveTexture;
    Texture normalTexture;

    Material_GLTF()
    {
        this->metallicFactor = 0.0f;
        this->roughnessFactor = 1.0f;
    }

    Material_GLTF(vector<Texture> textures, float metallicFactor = 0.0f, float roughnessFactor = 1.0f)
    {
        for (int i = 0; i < textures.size(); i++)
        {
            switch (textures[i].type)
            {
            case TextureType::BaseColor:
                baseColorTexture = textures[i];
                break;
            case TextureType::MetallicRoughness:
                metallicRoughnessTexture = textures[i];
                break;
            case TextureType::Emissive:
                emissiveTexture = textures[i];
                break;
            case TextureType::Normal:
                normalTexture = textures[i];
                break;
            default:
                break;
            }
        }

        this->metallicFactor = metallicFactor;
        this->roughnessFactor = roughnessFactor;
    }
};

class Mesh_GLTF
{
public:
    //Mesh Data
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    Material_GLTF material;
    unsigned int VAO;

    // constructor
    Mesh_GLTF(vector<Vertex> vertices, vector<unsigned int> indices, Material_GLTF material)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->material = material;

        // now that we have all the required data, set the vertex buffers and its attribute pointers.
        setupMesh();
    }

    // render the mesh without any texturing.
    void SimpleDraw(Shader& shader, mat4 meshMatrix)
    {
        //Set The Model Uniform
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, value_ptr(meshMatrix));

        // draw mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // render the mesh
    void Draw(Shader& shader, mat4 meshMatrix)
    {
        //Cache Sizes of All Textures.
        int baseColorTextureOffset = material.baseColorTexture.type != TextureType::None ? 1 : 0;
        int metallicRoughnessTextureOffset = material.metallicRoughnessTexture.type != TextureType::None ? 1 : 0;
        int emissiveTextureOffset = material.emissiveTexture.type != TextureType::None ? 1 : 0;
        int normalTextureOffset = material.normalTexture.type != TextureType::None ? 1 : 0;

        if (baseColorTextureOffset)
        {
            //Bind Base Color Texture!
            glActiveTexture(GL_TEXTURE0);
            glUniform1i(glGetUniformLocation(shader.ID, "material.baseColorTexture"), 0);
            glUniform1ui(glGetUniformLocation(shader.ID, "material.hasBCT"), 1);
            glBindTexture(GL_TEXTURE_2D, material.baseColorTexture.ID);
        }

        if (metallicRoughnessTextureOffset)
        {
            //Bind All Metallic Roughness Textures!
            glActiveTexture(GL_TEXTURE0 + baseColorTextureOffset);
            glUniform1i(glGetUniformLocation(shader.ID, "material.metallicRoughnessTexture"), baseColorTextureOffset);
            glUniform1ui(glGetUniformLocation(shader.ID, "material.hasMRT"), 1);
            glBindTexture(GL_TEXTURE_2D, material.metallicRoughnessTexture.ID);
        }

        if (emissiveTextureOffset)
        {
            //Bind All Emissive Textures!
            int offset = baseColorTextureOffset + metallicRoughnessTextureOffset;
            glActiveTexture(GL_TEXTURE0 + offset);
            glUniform1i(glGetUniformLocation(shader.ID, "material.emissionTexture"), offset);
            glUniform1ui(glGetUniformLocation(shader.ID, "material.hasET"), 1);
            glBindTexture(GL_TEXTURE_2D, material.emissiveTexture.ID);
        }
        
        if (normalTextureOffset)
        {
            int offset = baseColorTextureOffset + metallicRoughnessTextureOffset + emissiveTextureOffset;
            glActiveTexture(GL_TEXTURE0 + offset);
            glUniform1i(glGetUniformLocation(shader.ID, "material.normalTexture"), offset);
            glUniform1ui(glGetUniformLocation(shader.ID, "material.hasNT"), 1);
            glBindTexture(GL_TEXTURE_2D, material.normalTexture.ID);
        }

        //Set The Additional Material Properties.
        glUniform1f(glGetUniformLocation(shader.ID, "material.metallicFactor"), material.metallicFactor);
        glUniform1f(glGetUniformLocation(shader.ID, "material.roughnessFactor"), material.roughnessFactor);

        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, value_ptr(meshMatrix));
        // draw mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUniform1ui(glGetUniformLocation(shader.ID, "material.hasBCT"), 0);
        glUniform1ui(glGetUniformLocation(shader.ID, "material.hasMRT"), 0);
        glUniform1ui(glGetUniformLocation(shader.ID, "material.hasET"), 0);
        glUniform1ui(glGetUniformLocation(shader.ID, "material.hasNT"), 0);

        // always good practice to set everything back to defaults once configured.
        glActiveTexture(GL_TEXTURE0);
    }

private:
    // render data 
    unsigned int VBO, EBO;

    // initializes all the buffer objects/arrays
    void setupMesh()
    {
        // create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // vertex tangent
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // vertex texture coords
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoord));

        glBindVertexArray(0);
    }
};


#endif