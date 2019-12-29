#pragma once

#include "MaterialManager.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

using namespace std;

//Holds Vertex + Index data on GPU and CPU
struct MeshData
{
	Buffer vertexBuffer;
	Buffer indexBuffer;

	template<typename T> explicit MeshData(VkPhysicalDevice physicalDevice, VkDevice device, vector<T> const& vData, vector<unsigned int> const& iData):
		vertexBuffer(physicalDevice, device, &vData[0], vData.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
		indexBuffer(physicalDevice, device, &iData[0], iData.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
	{
	}

};

struct ModelData
{
	vector<MeshData> meshes;
	vector<shared_ptr<const MaterialDescription>> materials;
	string path;
};