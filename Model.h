#pragma once

#include "MaterialManager.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

using namespace std;

//Holds Vertex + Index data on GPU
struct MeshData
{
	VkBuffer vertexBufffer;
	VkDeviceMemory vertexBufferMemory;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	size_t indexBufferSize;

	VkDevice m_Device = VK_NULL_HANDLE;

	template<typename T> explicit MeshData(VkPhysicalDevice physicalDevice, VkDevice device, vector<T> const& vData, vector<unsigned int> const& iData)
	{
		m_Device = device;
		indexBufferSize = iData.size();

		//Send loaded data to GPU
		VulkanHelpers::createBuffer(vertexBufffer, vertexBufferMemory, physicalDevice, device, &vData[0], vData.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		VulkanHelpers::createBuffer(indexBuffer, indexBufferMemory, physicalDevice, device, &iData[0], iData.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	}

	~MeshData()
	{
		destroy();
	}

	MeshData& operator=(MeshData&& obj)
	{
		destroy();
		*this = obj;
		obj.m_Device = VK_NULL_HANDLE;

		return *this;
	}

	MeshData(MeshData&& obj)
	{
		*this = move(obj);
	}

private:
	MeshData& operator=(MeshData const&) = default;
	MeshData(MeshData const&) = default;

	void destroy()
	{
		if (m_Device == VK_NULL_HANDLE) return;

		vkDestroyBuffer(m_Device, vertexBufffer, nullptr);
		vkFreeMemory(m_Device, vertexBufferMemory, nullptr);

		vkDestroyBuffer(m_Device, indexBuffer, nullptr);
		vkFreeMemory(m_Device, indexBufferMemory, nullptr);

		m_Device = VK_NULL_HANDLE;
	}
};

struct ModelData
{
	vector<MeshData> meshes;
	vector<shared_ptr<const MaterialDescription>> materials;
	string path;
};