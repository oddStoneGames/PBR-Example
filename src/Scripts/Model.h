#ifndef Model_H
#define Model_H

#include "json.h"
#include "Mesh.h"

using json = nlohmann::json;

// Reads a text file and outputs a string with everything in the text file
std::string get_file_contents(const char* filename);

class Model
{
public:
	// Loads in a model from a file and stores tha information in 'data', 'JSON', and 'file'
	Model(const char* file);
	void Draw(Shader& shader, mat4 model);
	void SimpleDraw(Shader& shader, mat4 model);

	// All the meshes and transformations
	std::vector<Mesh_GLTF> meshes;

private:
	// Variables for easy access
	const char* file;
	std::vector<unsigned char> data;
	json JSON;
	std::vector<glm::mat4> matricesMeshes;

	// The Default Rotation To Align Model as Front Facing(By Rotation of 270 degrees in the Y Axis)
	glm::mat4 blenderImportRotation;

	// Prevents textures from being loaded twice
	std::vector<std::string> loadedTexName;
	std::vector<Texture> loadedTex;

	// Loads a single mesh by its index
	void loadMesh(unsigned int indMesh);

	// Traverses a node recursively, so it essentially traverses all connected nodes
	void traverseNode(unsigned int nextNode, glm::mat4 matrix = glm::mat4(1.0f));

	// Gets the binary data from a file
	std::vector<unsigned char> getData();
	// Interprets the binary data into floats, indices, and textures
	std::vector<float> getFloats(json accessor);
	std::vector<GLuint> getIndices(json accessor);

	// Assembles all the floats into vertices
	std::vector<Vertex> assembleVertices
	(
		std::vector<glm::vec3> positions,
		std::vector<glm::vec3> normals,
		std::vector<glm::vec3> tangents,
		std::vector<glm::vec2> texUVs
	);

	// Helps with the assembly from above by grouping floats
	std::vector<glm::vec2> groupFloatsVec2(std::vector<float> floatVec);
	std::vector<glm::vec3> groupFloatsVec3(std::vector<float> floatVec);
	std::vector<glm::vec4> groupFloatsVec4(std::vector<float> floatVec);
	std::vector<glm::vec3> groupFloatsVec4asVec3(std::vector<float> floatVec);
};
#endif