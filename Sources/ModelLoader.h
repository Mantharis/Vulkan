#pragma once

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <vector>
#include <unordered_map>
using namespace std;

struct DataPerMesh
{
	vector<float> vertices;
	vector<float> texCoords;
	vector<float> normals;
	vector<unsigned int> indices;
	tinyobj::material_t material;
};


class ModelLoader
{
public:

	static long long calculateHash(unsigned int a, unsigned int b, unsigned int c, unsigned int maxA, unsigned int maxB)
	{
		return ((long long)a) * maxA * maxB + (long long)b * maxB + c;
	}

	static bool loadFromFile(const char* modelPath, const char* materialPath, vector< DataPerMesh>& outMeshData)
	{
		tinyobj::attrib_t attrib;
		vector<tinyobj::shape_t> shapes;
		vector<tinyobj::material_t> materials;

		string err;
		bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath, materialPath, true);

		if (!ret) return false;


		for (size_t materialId = 0; materialId < materials.size(); ++materialId)
		{

			unordered_map<long long, unsigned int> indexMapping;

			unsigned int indexCnt = 0;
			outMeshData.push_back(DataPerMesh());
			DataPerMesh& mesh = outMeshData.back();

			mesh.material = materials[materialId];

			for (auto& shape : shapes)
			{
				for (size_t faceId = 0; faceId < shape.mesh.material_ids.size(); ++faceId)
				{
					if (shape.mesh.material_ids[faceId] == materialId)
					{
						for (int v = 0; v < 3; ++v)
						{
							auto& index = shape.mesh.indices[faceId * 3 + v];

							pair<long long, unsigned int> keyValue = make_pair(calculateHash(index.vertex_index, index.texcoord_index, index.normal_index, 100000, 100000), indexCnt);
							auto pairItInserted = indexMapping.insert(keyValue);

							if (pairItInserted.second)
							{

								
								mesh.normals.push_back(attrib.normals[index.normal_index * 3]);
								mesh.normals.push_back(attrib.normals[index.normal_index * 3 + 1]);
								mesh.normals.push_back(attrib.normals[index.normal_index * 3 + 2]);

								mesh.vertices.push_back(attrib.vertices[index.vertex_index * 3]);
								mesh.vertices.push_back(attrib.vertices[index.vertex_index * 3 + 1]);
								mesh.vertices.push_back(attrib.vertices[index.vertex_index * 3 + 2]);

								mesh.texCoords.push_back(attrib.texcoords[max(0, index.texcoord_index * 2)]);
								mesh.texCoords.push_back(attrib.texcoords[max(0, index.texcoord_index * 2 + 1)]);

								++indexCnt;
							}

							mesh.indices.push_back(pairItInserted.first->second);
						}

					}
				}

			}
		}


		return true;
	}
};