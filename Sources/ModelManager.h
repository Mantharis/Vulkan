#pragma once

#include "Model.h"
#include "ModelLoader.h"
#include "VertexFormat.h"

#include <memory>
#include <assert.h>
#include <unordered_map>
#include <vector>

using ModelDataSharedPtr = shared_ptr<const ModelData>;

class ModelManager
{
public:

	ModelManager(VkPhysicalDevice physicalDevice, VkDevice device, MaterialManager& materialManager):m_MaterialManager(materialManager)
	{
		m_PhysicalDevice = physicalDevice;
		m_Device = device;
	}

	~ModelManager()
	{
		assert(m_Models.size() == 0 && "ModelManager cannot be destroyed before models are released!");
	}

	ModelDataSharedPtr loadModel(string const& path)
	{
		auto it = m_Models.find(path);
		if (it != end(m_Models))
		{
			return ModelDataSharedPtr(it->second);
		}
		else
		{
			ModelData* newModel = new ModelData();
			newModel->path = path;

			//determine material file path from obj path (same path just different sufix)

			auto dirPath = path.find_last_of("/\\");

			string materialDirPath;
			if (dirPath != path.npos)
			{
				materialDirPath = path.substr(0, dirPath);
			}

			newModel->path = path;

			//Load data from file
			vector<DataPerMesh> dataPerMesh;
			ModelLoader::loadFromFile(path.c_str(), materialDirPath.c_str(), dataPerMesh);
			for (auto& mesh : dataPerMesh)
			{
				vector<Vertex> vertices;

				for (size_t idx = 0; idx < mesh.vertices.size() / 3; ++idx)
				{
					Vertex vertex;
					vertex.pos = glm::vec3(mesh.vertices[idx * 3], mesh.vertices[idx * 3 + 1], mesh.vertices[idx * 3 + 2]);
					vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
					vertex.texCoord = glm::vec2(mesh.texCoords[idx * 2], 1.0f - mesh.texCoords[idx * 2 + 1]);

					if (!mesh.normals.empty())
					{
						vertex.normal = glm::vec3(mesh.normals[idx * 3], mesh.normals[idx * 3 + 1], mesh.normals[idx * 3 + 2]);
					}

					vertices.push_back(vertex);
				}

				//Calculate Tangent and BiTangent and Normal (if not available)
				for (size_t idx = 0; idx < mesh.indices.size(); idx+=3)
				{
					Vertex& vert0 = vertices[mesh.indices[idx]];
					Vertex& vert1 = vertices[mesh.indices[idx+1]];
					Vertex& vert2 = vertices[mesh.indices[idx+2]];

					// Edges of the triangle : position delta
					glm::vec3 deltaPos1 = vert1.pos - vert0.pos;
					glm::vec3 deltaPos2 = vert2.pos - vert0.pos;

					// UV delta
					glm::vec2 deltaUV1 = vert1.texCoord - vert0.texCoord;
					glm::vec2 deltaUV2 = vert2.texCoord - vert0.texCoord;

					float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
					glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
					glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

					if (mesh.normals.empty())
					{
						glm::vec3 normal = normalize(glm::cross(deltaPos1, deltaPos2 * -1.0f));
						vert0.normal = vert1.normal = vert2.normal = normal;
					}

					vert0.tangent = vert1.tangent = vert2.tangent = tangent;
					vert0.bitangent = vert1.bitangent = vert2.bitangent = bitangent;
				}

				MeshData meshData(m_PhysicalDevice, m_Device, vertices, mesh.indices);
				newModel->meshes.push_back(move(meshData));

				auto materialData = m_MaterialManager.createMaterial(mesh.material, materialDirPath);
				newModel->materials.push_back(move(materialData));
			}

			ModelDataSharedPtr sharedPtr(newModel, [this](ModelData* modelData)
				{
					//remove from database
					m_Models.erase(modelData->path);

					//free CPU memory
					delete modelData;
				});

			//add into database
			m_Models[path] = sharedPtr;

			return sharedPtr;
		}
	}

private:
	MaterialManager& m_MaterialManager;

	unordered_map<string, weak_ptr <const ModelData> > m_Models;
	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_Device;
};