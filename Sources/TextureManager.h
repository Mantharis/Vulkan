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
	TextureManager(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue graphicQueue)
	{
		m_PhysicalDevice = physicalDevice;
		m_Device = device;
		m_CommandPool = commandPool;
		m_GraphicQueue = graphicQueue;
	}

	~TextureManager()
	{
		assert(m_TextureData.size() == 0 && "TextureManager cannot be destroyed before textures are released!");
	}

	TextureDataSharedPtr loadTexture(string const& path)
	{
		auto it = m_TextureData.find(path);
		if (it != end(m_TextureData))
		{
			return TextureDataSharedPtr(it->second);
		}
		else
		{
			TextureData* newTexture = new TextureData();
			newTexture->path = path;

			VulkanHelpers::createTextureImage(m_PhysicalDevice, m_CommandPool, m_GraphicQueue, newTexture->image, newTexture->deviceMemory, path.c_str(), m_Device, VK_FORMAT_R8G8B8A8_UNORM);
			VulkanHelpers::createImageView(newTexture->imageView, m_Device, newTexture->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

			TextureDataSharedPtr sharedPtr(newTexture, [this](TextureData* texData)
				{
					//free GPU memory
					vkDestroyImageView(m_Device, texData->imageView, nullptr);
					vkDestroyImage(m_Device, texData->image, nullptr);
					vkFreeMemory(m_Device, texData->deviceMemory, nullptr);

					//remove from database
					m_TextureData.erase(texData->path);

					//free CPU memory
					delete texData;
				});

			//add into database
			m_TextureData[path] = sharedPtr;

			return sharedPtr;
		}

	}

private:
	VkPhysicalDevice m_PhysicalDevice;
	VkCommandPool m_CommandPool;
	VkQueue m_GraphicQueue;
	VkDevice m_Device;

	unordered_map<string, weak_ptr <const TextureData> > m_TextureData;
};