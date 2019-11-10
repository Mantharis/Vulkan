#pragma once

#include <tiny_obj_loader.h>

#include <vector>
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
	static long long calculateHash(unsigned int a, unsigned int b, unsigned int c, unsigned int maxA, unsigned int maxB);
	static bool loadFromFile(const char* modelPath, const char* materialPath, vector< DataPerMesh>& outMeshData);
};