#pragma once

#include "VulkanHelper.h"

#include <string>
#include <unordered_map>
#include <memory>
using namespace std;

struct TextureData
{
	VkImageView imageView;
	VkImage image;
	VkDeviceMemory deviceMemory;
	string path;
};

using TextureDataSharedPtr = shared_ptr<const TextureData>;

class TextureManager
{
public:
	TextureManager(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue graphicQueue);
	~TextureManager();
	TextureDataSharedPtr loadTexture(string const& path);

private:
	VkPhysicalDevice m_PhysicalDevice;
	VkCommandPool m_CommandPool;
	VkQueue m_GraphicQueue;
	VkDevice m_Device;

	unordered_map<string, weak_ptr <const TextureData> > m_TextureData;
};