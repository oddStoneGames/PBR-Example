#define STB_IMAGE_IMPLEMENTATION
#include "Model.h"

// Reads a text file and outputs a string with everything in the text file
std::string get_file_contents(const char* filename)
{
	std::ifstream in(filename, std::ios::binary);
	if (in)
	{
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}
	throw(errno);
}

Model::Model(const char* file)
{
	// Make a JSON object
	std::string text = get_file_contents(file);
	JSON = json::parse(text);

	// Get the binary data
	Model::file = file;
	data = getData();

	//Initialize Default Blender Import Rotation.
	blenderImportRotation = glm::mat4(1.0f);
	blenderImportRotation = glm::rotate(blenderImportRotation, glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	// Traverse all nodes
	for(volatile int i = 0; i < JSON["nodes"].size(); i++)
		traverseNode(i);
}

void Model::SimpleDraw(Shader& shader, mat4 model)
{
	// Go over all meshes and draw each one without any texturing.
	for (volatile unsigned int i = 0; i < meshes.size(); i++)
		meshes[i].Mesh_GLTF::SimpleDraw(shader, model * matricesMeshes[i] * blenderImportRotation);
}

void Model::Draw(Shader& shader, mat4 model)
{
	// Go over all meshes and draw each one
	for (volatile unsigned int i = 0; i < meshes.size(); i++)
		meshes[i].Mesh_GLTF::Draw(shader, model * matricesMeshes[i] * blenderImportRotation);
}

void Model::loadMesh(unsigned int indMesh)
{
	// Get all accessor indices
	unsigned int posAccInd     = JSON["meshes"][indMesh]["primitives"][0]["attributes"]["POSITION"];
	unsigned int normalAccInd  = JSON["meshes"][indMesh]["primitives"][0]["attributes"]["NORMAL"];
	unsigned int tangentAccInd = JSON["meshes"][indMesh]["primitives"][0]["attributes"]["TANGENT"];
	unsigned int texAccInd     = JSON["meshes"][indMesh]["primitives"][0]["attributes"]["TEXCOORD_0"];
	unsigned int indAccInd     = JSON["meshes"][indMesh]["primitives"][0]["indices"];

	// Use accessor indices to get all vertices components
	std::vector<float> posVec = getFloats(JSON["accessors"][posAccInd]);
	std::vector<glm::vec3> positions = groupFloatsVec3(posVec);
	std::vector<float> normalVec = getFloats(JSON["accessors"][normalAccInd]);
	std::vector<glm::vec3> normals = groupFloatsVec3(normalVec);
	std::vector<float> tangentVec = getFloats(JSON["accessors"][tangentAccInd]);
	std::vector<glm::vec3> tangents = groupFloatsVec4asVec3(tangentVec);
	std::vector<float> texVec = getFloats(JSON["accessors"][texAccInd]);
	std::vector<glm::vec2> texUVs = groupFloatsVec2(texVec);

	// Combine all the vertex components and also get the indices and textures
	std::vector<Vertex> vertices = assembleVertices(positions, normals, tangents, texUVs);
	std::vector<GLuint> indices = getIndices(JSON["accessors"][indAccInd]);

	//Load The Material For This Mesh!
	//Blender Does textures in Former Way 
	//Emissive - 0, Normal - 1, baseColor - 2, metallicRoughness - 3

	//Get Emissive Texture Index if it exists.
	unsigned int hasEmissiveTexture = JSON["materials"][indMesh].contains("emissiveTexture");
	int emissiveTextureIndex = hasEmissiveTexture ? (int)(JSON["materials"][indMesh]["emissiveTexture"]["index"]) : -1;

	//Get Normal Texture Index if it exists.
	unsigned int hasNormalTexture = JSON["materials"][indMesh].contains("normalTexture");
	int normalTextureIndex = hasNormalTexture ? (int)(JSON["materials"][indMesh]["normalTexture"]["index"]) : -1;

	//Get Base Color Texture Index if it exists.
	unsigned int hasBaseColorTexture = JSON["materials"][indMesh]["pbrMetallicRoughness"].contains("baseColorTexture");
	int baseColorTextureIndex = hasBaseColorTexture ? (int)(JSON["materials"][indMesh]["pbrMetallicRoughness"]["baseColorTexture"]["index"]) : -1;

	//Get Metallic Roughness Texture Index if it exists.
	unsigned int hasMetallicRoughnessTexture = JSON["materials"][indMesh]["pbrMetallicRoughness"].contains("metallicRoughnessTexture");
	int metallicRoughnessTextureIndex = hasMetallicRoughnessTexture ? (int)(JSON["materials"][indMesh]["pbrMetallicRoughness"]["metallicRoughnessTexture"]["index"]) : -1;

	//Get Metallic Factor if it exists.
	unsigned int hasMetallicFactor = JSON["materials"][indMesh]["pbrMetallicRoughness"].contains("metallicFactor");
	float metallicFactor = hasMetallicFactor ? (float)(JSON["materials"][indMesh]["pbrMetallicRoughness"]["metallicFactor"]) : -1.0f;

	//Get Roughness Factor if it exists.
	unsigned int hasRoughnessFactor = JSON["materials"][indMesh]["pbrMetallicRoughness"].contains("roughnessFactor");
	float roughnessFactor = hasRoughnessFactor ? (float)(JSON["materials"][indMesh]["pbrMetallicRoughness"]["roughnessFactor"]) : -1.0f;

	//Our Convention For Textures Are - 
	//Base Color - 0, Metallic Roughness - 1, Emissive - 2, Normal - 3

	vector<Texture> textures;

	//Get The Current File Directory.
	std::string fileStr = std::string(file);
	std::string fileDirectory = fileStr.substr(0, fileStr.find_last_of('/') + 1);

	//Load Base Color Texture.
	if (hasBaseColorTexture)
	{
		string textureName = JSON["images"][baseColorTextureIndex]["uri"];
		Texture baseColorTexture((fileDirectory + textureName).c_str(), TextureType::BaseColor, 0);
		textures.push_back(baseColorTexture);
	}

	//Load Metallic Roughness Texture.
	if (hasMetallicRoughnessTexture)
	{
		string textureName = JSON["images"][metallicRoughnessTextureIndex]["uri"];
		Texture metallicRoughnessTexture((fileDirectory + textureName).c_str(), TextureType::MetallicRoughness, hasBaseColorTexture);
		textures.push_back(metallicRoughnessTexture);
	}

	//Load Emissive Texture.
	if (hasEmissiveTexture)
	{
		string textureName = JSON["images"][emissiveTextureIndex]["uri"];
		Texture emissiveTexture((fileDirectory + textureName).c_str(), TextureType::Emissive, hasBaseColorTexture + hasMetallicRoughnessTexture);
		textures.push_back(emissiveTexture);
	}

	//Load Normal Texture.
	if (hasNormalTexture)
	{
		string textureName = JSON["images"][normalTextureIndex]["uri"];
		Texture normalTexture((fileDirectory + textureName).c_str(), TextureType::Normal, hasBaseColorTexture + hasMetallicRoughnessTexture + hasEmissiveTexture);
		textures.push_back(normalTexture);
	}

	//Create Material!
	Material_GLTF material(textures, metallicFactor, roughnessFactor);

	// Combine the vertices, indices, and Material into a Mesh_GLTF
	meshes.push_back(Mesh_GLTF(vertices, indices, material));
}

void Model::traverseNode(unsigned int nextNode, glm::mat4 matrix)
{
	// Current node
	json node = JSON["nodes"][nextNode];

	// Get translation if it exists
	glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);
	if (node.find("translation") != node.end())
	{
		float transValues[3];
		for (volatile unsigned int i = 0; i < node["translation"].size(); i++)
			transValues[i] = (node["translation"][i]);

		translation = glm::make_vec3(transValues);
	}
	
	// Get quaternion if it exists
	glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	if (node.find("rotation") != node.end())
	{
		float rotValues[4] =
		{
			node["rotation"][0],
			node["rotation"][1],
			node["rotation"][2],
			node["rotation"][3]
		};
		
		rotation = glm::make_quat(rotValues);
	}
	// Get scale if it exists
	glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
	if (node.find("scale") != node.end())
	{
		float scaleValues[3];
		for (volatile unsigned int i = 0; i < node["scale"].size(); i++)
			scaleValues[i] = (node["scale"][i]);
		scale = glm::make_vec3(scaleValues);
	}

	// Initialize matrices
	glm::mat4 trans = glm::mat4(1.0f);
	glm::mat4 rot = glm::mat4(1.0f);
	glm::mat4 sca = glm::mat4(1.0f);

	// Use translation, rotation, and scale to change the initialized matrices
	trans = glm::translate(trans, translation);
	rot = glm::mat4_cast(rotation);
	sca = glm::scale(sca, scale);

	// Multiply all matrices together
	glm::mat4 matNextNode = matrix * trans * rot * sca;

	// Check if the node contains a Mesh_GLTF and if it does load it
	if (node.find("mesh") != node.end())
	{
		matricesMeshes.push_back(matNextNode);
		loadMesh(node["mesh"]);
	}

	// Check if the node has children, and if it does, apply this function to them with the matNextNode
	if (node.find("children") != node.end())
	{
		for (volatile unsigned int i = 0; i < node["children"].size(); i++)
			traverseNode(node["children"][i], matNextNode);
	}
}

std::vector<unsigned char> Model::getData()
{
	// Create a place to store the raw text, and get the uri of the .bin file
	std::string bytesText;
	std::string uri = JSON["buffers"][0]["uri"];

	// Store raw text data into bytesText
	std::string fileStr = std::string(file);
	std::string fileDirectory = fileStr.substr(0, fileStr.find_last_of('/') + 1);
	bytesText = get_file_contents((fileDirectory + uri).c_str());

	// Transform the raw text data into bytes and put them in a vector
	std::vector<unsigned char> data(bytesText.begin(), bytesText.end());
	return data;
}

std::vector<float> Model::getFloats(json accessor)
{
	std::vector<float> floatVec;

	// Get properties from the accessor
	unsigned int buffViewInd = accessor.value("bufferView", 1);
	unsigned int count = accessor["count"];
	unsigned int accByteOffset = accessor.value("byteOffset", 0);
	std::string type = accessor["type"];

	// Get properties from the bufferView
	json bufferView = JSON["bufferViews"][buffViewInd];
	unsigned int byteOffset = bufferView["byteOffset"];

	// Interpret the type and store it into numPerVert
	unsigned int numPerVert;
	if (type == "SCALAR") numPerVert = 1;
	else if (type == "VEC2") numPerVert = 2;
	else if (type == "VEC3") numPerVert = 3;
	else if (type == "VEC4") numPerVert = 4;
	else throw std::invalid_argument("Type is invalid (not SCALAR, VEC2, VEC3, or VEC4)");

	// Go over all the bytes in the data at the correct place using the properties from above
	unsigned int beginningOfData = byteOffset + accByteOffset;
	unsigned int lengthOfData = count * 4 * numPerVert;
	for (volatile unsigned int i = beginningOfData; i < beginningOfData + lengthOfData; i)
	{
		unsigned char bytes[] = { data[i++], data[i++], data[i++], data[i++] };
		float value;
		std::memcpy(&value, bytes, sizeof(float));
		floatVec.push_back(value);
	}

	return floatVec;
}

std::vector<GLuint> Model::getIndices(json accessor)
{
	std::vector<GLuint> indices;

	// Get properties from the accessor
	unsigned int buffViewInd = accessor.value("bufferView", 0);
	unsigned int count = accessor["count"];
	unsigned int accByteOffset = accessor.value("byteOffset", 0);
	unsigned int componentType = accessor["componentType"];

	// Get properties from the bufferView
	json bufferView = JSON["bufferViews"][buffViewInd];
	unsigned int byteOffset = bufferView["byteOffset"];

	// Get indices with regards to their type: unsigned int, unsigned short, or short
	unsigned int beginningOfData = byteOffset + accByteOffset;
	if (componentType == 5125)
	{
		for (volatile unsigned int i = beginningOfData; i < byteOffset + accByteOffset + count * 4; i)
		{
			unsigned char bytes[] = { data[i++], data[i++], data[i++], data[i++] };
			unsigned int value;
			std::memcpy(&value, bytes, sizeof(unsigned int));
			indices.push_back((GLuint)value);
		}
	}
	else if (componentType == 5123)
	{
		for (volatile unsigned int i = beginningOfData; i < byteOffset + accByteOffset + count * 2; i)
		{
			unsigned char bytes[] = { data[i++], data[i++] };
			unsigned short value;
			std::memcpy(&value, bytes, sizeof(unsigned short));
			indices.push_back((GLuint)value);
		}
	}
	else if (componentType == 5122)
	{
		for (volatile unsigned int i = beginningOfData; i < byteOffset + accByteOffset + count * 2; i)
		{
			unsigned char bytes[] = { data[i++], data[i++] };
			short value;
			std::memcpy(&value, bytes, sizeof(short));
			indices.push_back((GLuint)value);
		}
	}

	return indices;
}

std::vector<Vertex> Model::assembleVertices(std::vector<glm::vec3> positions, std::vector<glm::vec3> normals, std::vector<glm::vec3> tangents, std::vector<glm::vec2> texUVs)
{
	std::vector<Vertex> vertices;
	for (volatile int i = 0; i < positions.size(); i++)
		vertices.push_back( Vertex { positions[i], normals[i], tangents[i], texUVs[i]} );
	return vertices;
}

std::vector<glm::vec2> Model::groupFloatsVec2(std::vector<float> floatVec)
{
	std::vector<glm::vec2> vectors;
	for (volatile int i = 0; i < floatVec.size(); i)
	{
		vectors.push_back(glm::vec2(floatVec[i++], floatVec[i++]));
	}
	return vectors;
}
std::vector<glm::vec3> Model::groupFloatsVec3(std::vector<float> floatVec)
{
	std::vector<glm::vec3> vectors;
	for (volatile int i = 0; i < floatVec.size(); i)
	{
		vectors.push_back(glm::vec3(floatVec[i++], floatVec[i++], floatVec[i++]));
	}
	return vectors;
}
std::vector<glm::vec4> Model::groupFloatsVec4(std::vector<float> floatVec)
{
	std::vector<glm::vec4> vectors;
	for (volatile int i = 0; i < floatVec.size(); i)
	{
		vectors.push_back(glm::vec4(floatVec[i++], floatVec[i++], floatVec[i++], floatVec[i++]));
	}
	return vectors;
}

std::vector<glm::vec3> Model::groupFloatsVec4asVec3(std::vector<float> floatVec)
{
	std::vector<glm::vec3> vectors;
	for (volatile int i = 0; i < floatVec.size(); i)
	{
		glm::vec4 temp = glm::vec4(floatVec[i++], floatVec[i++], floatVec[i++], floatVec[i++]);
		vectors.push_back(glm::vec3(temp.x, temp.y, temp.z));
	}
	return vectors;
}