#pragma once


#include "VulkanHelper.h"
#include "TextureManager.h"

#include <tiny_obj_loader.h>

#include <vector>
#include <array>
#include <memory>
#include <variant>

using namespace std;



struct MaterialUBO
{
	alignas(16) glm::vec3 diffuse;
	alignas(16) glm::vec3 ambient;
	alignas(16) glm::vec3 specular;
	alignas(4) float shininess;
};

struct Sampler
{
	VkSampler m_Sampler;
	VkDevice m_Device = VK_NULL_HANDLE;

	Sampler(VkDevice device)
	{
		m_Device = device;
		VulkanHelpers::createTextureSampler(m_Sampler, m_Device);
	}

	~Sampler()
	{
		destroy();
	}

	Sampler& operator=(Sampler&& obj)
	{
		destroy();
		*this = obj;
		obj.m_Device = VK_NULL_HANDLE;

		return *this;
	}

	Sampler(Sampler&& obj)
	{
		*this = move(obj);
	}

private:
	Sampler& operator=(Sampler const&) = default;
	Sampler(Sampler const&) = default;

	void destroy()
	{
		if (m_Device == VK_NULL_HANDLE) return;

		vkDestroySampler(m_Device, m_Sampler, nullptr);
		m_Device = VK_NULL_HANDLE;
	}
};

struct Buffer
{
	VkBuffer buffer;
	VkDeviceMemory bufferMemory;

	VkDevice m_Device = VK_NULL_HANDLE;

	template<typename T> explicit Buffer(VkPhysicalDevice physicalDevice, VkDevice device, T* bufferDataPtr,size_t  dataCnt, VkBufferUsageFlagBits usageBits)
	{
		m_Device = device;

		VulkanHelpers::createBuffer(buffer, bufferMemory, physicalDevice, device, bufferDataPtr, dataCnt, usageBits);
	}

	~Buffer()
	{
		destroy();
	}

	Buffer& operator=(Buffer&& obj)
	{
		destroy();
		*this = obj;
		obj.m_Device = VK_NULL_HANDLE;

		return *this;
	}

	Buffer(Buffer&& obj)
	{
		*this = move(obj);
	}

	template<typename T> void updateBuffer(T* bufferDataPtr, size_t  dataCnt)
	{
		VulkanHelpers::copyData(bufferMemory, m_Device, bufferDataPtr, dataCnt);
	}
	

private:
	Buffer& operator=(Buffer const&) = default;
	Buffer(Buffer const&) = default;

	void destroy()
	{
		if (m_Device == VK_NULL_HANDLE) return;

		vkDestroyBuffer(m_Device, buffer, nullptr);
		vkFreeMemory(m_Device, bufferMemory, nullptr);
		m_Device = VK_NULL_HANDLE;
	}
};


struct DescriptorSet
{
	VkDescriptorPool m_DescriptorPool;
	VkDescriptorSet m_DescriptorSet;
	VkDevice m_Device = VK_NULL_HANDLE;

	DescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, vector<variant< VkDescriptorImageInfo, VkDescriptorBufferInfo>> const &descriptorWrites)
	{
		m_Device = device;

		unordered_multiset<VkDescriptorType> descriptorTypeCnt;

		for (auto& descriptorWrite : descriptorWrites)
		{
			if (get_if<VkDescriptorImageInfo>(&descriptorWrite)) descriptorTypeCnt.insert(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			else if (get_if<VkDescriptorBufferInfo>(&descriptorWrite)) descriptorTypeCnt.insert(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			else assert(!"Unsupported type");
		}

		vector< VkDescriptorPoolSize> poolSizes;
		for (auto& descriptorType : descriptorTypeCnt)
		{
			VkDescriptorPoolSize descriptorPoolSize;
			descriptorPoolSize.type = descriptorType;
			descriptorPoolSize.descriptorCount = descriptorTypeCnt.count(descriptorType);

			poolSizes.push_back(descriptorPoolSize);
		}

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = 1;

		auto res = vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool);
		assert(res == VK_SUCCESS);

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		res = vkAllocateDescriptorSets(device, &allocInfo, &m_DescriptorSet);

		assert(res == VK_SUCCESS);

		vector< VkWriteDescriptorSet> descriptorWriteSet;
		for (auto& descriptorWrite : descriptorWrites)
		{
			VkWriteDescriptorSet newDescriptorSet = {};

			if (auto imageInfo = get_if<VkDescriptorImageInfo>(&descriptorWrite))
			{
				newDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				newDescriptorSet.dstSet = m_DescriptorSet;
				newDescriptorSet.dstBinding = descriptorWriteSet.size();
				newDescriptorSet.dstArrayElement = 0;
				newDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				newDescriptorSet.descriptorCount = 1;
				newDescriptorSet.pImageInfo = imageInfo;
			}
			else if (auto bufferInfo = get_if<VkDescriptorBufferInfo>(&descriptorWrite))
			{
				newDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				newDescriptorSet.dstSet = m_DescriptorSet;
				newDescriptorSet.dstBinding = descriptorWriteSet.size();
				newDescriptorSet.dstArrayElement = 0;
				newDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				newDescriptorSet.descriptorCount = 1;
				newDescriptorSet.pBufferInfo = bufferInfo;
			}
			else
			{
				assert(!"Unsupported type");
			}

			descriptorWriteSet.push_back(newDescriptorSet);
		}

		vkUpdateDescriptorSets(device, descriptorWriteSet.size(), &descriptorWriteSet[0], 0, nullptr);
	}


	~DescriptorSet()
	{
		destroy();
	}

	DescriptorSet& operator=(DescriptorSet&& obj)
	{
		destroy();
		*this = obj;
		obj.m_Device = VK_NULL_HANDLE;

		return *this;
	}

	DescriptorSet(DescriptorSet&& obj)
	{
		*this = move(obj);
	}

private:
	DescriptorSet& operator=(DescriptorSet const&) = default;
	DescriptorSet(DescriptorSet const&) = default;

	void destroy()
	{
		if (m_Device == VK_NULL_HANDLE) return;

		vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
		m_Device = VK_NULL_HANDLE;
	}
};

struct MaterialDescription
{
	shared_ptr<const TextureData> m_DiffuseTexture;
	shared_ptr<const TextureData> m_ReflectionTexture;

	shared_ptr<const Sampler> m_Sampler;


	Buffer m_UniformBuffer;
	DescriptorSet m_DescriptorSet;
	string m_Path;
};


class MaterialManager
{
public:

	MaterialManager(TextureManager& textureManager, VkDevice device, VkPhysicalDevice physicalDevice, VkDescriptorSetLayout descriptorSetLayout) :m_TextureManager(textureManager)
	{
		m_Device = device;
		m_PhysicalDevice = physicalDevice;
		m_DescriptorSetLayout = descriptorSetLayout;
	}
	shared_ptr<const MaterialDescription> createMaterial(tinyobj::material_t const& material, const string& path)
	{
		auto it = m_Data.find(material.diffuse_texname);
		if (it != end(m_Data))
		{
			return shared_ptr<const MaterialDescription>(it->second);
		}
		else
		{
			auto diffuseTexture = m_TextureManager.loadTexture(path + "/" + material.diffuse_texname);
			auto reflectionTexture = m_TextureManager.loadTexture(path + "/" + material.reflection_texname);

			auto sampler = shared_ptr<const Sampler>(new Sampler(m_Device)); //TODO

			MaterialUBO materialBuffer;
			materialBuffer.diffuse.r = material.diffuse[0];
			materialBuffer.diffuse.g = material.diffuse[1];
			materialBuffer.diffuse.b = material.diffuse[2];

			materialBuffer.ambient.r = material.ambient[0];
			materialBuffer.ambient.g = material.ambient[1];
			materialBuffer.ambient.b = material.ambient[2];

			materialBuffer.specular.r = material.specular[0];
			materialBuffer.specular.g = material.specular[1];
			materialBuffer.specular.b = material.specular[2];

			materialBuffer.shininess = material.shininess;

		
			auto buffer = Buffer(m_PhysicalDevice, m_Device, &materialBuffer, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = buffer.buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(MaterialUBO);

			VkDescriptorImageInfo imageInfo1 = {};
			imageInfo1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo1.imageView = diffuseTexture->imageView;
			imageInfo1.sampler = sampler->m_Sampler;

			VkDescriptorImageInfo imageInfo2 = {};
			imageInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo2.imageView = reflectionTexture->imageView;
			imageInfo2.sampler = sampler->m_Sampler;

			DescriptorSet descriptorSet(m_Device, m_DescriptorSetLayout, { bufferInfo, imageInfo1, imageInfo2 });

			MaterialDescription* newMaterial = new MaterialDescription{ move(diffuseTexture), move(reflectionTexture), move(sampler), move(buffer), move(descriptorSet) };
			newMaterial->m_Path = material.diffuse_texname;

			shared_ptr<const MaterialDescription> sharedPtr(newMaterial, [this](MaterialDescription* matData)
				{
					//remove from database
					m_Data.erase(matData->m_Path);

					//free CPU memory
					delete matData;
				});

			//add into database
			m_Data[material.diffuse_texname] = sharedPtr;

			return sharedPtr;
		}
	}

private:
	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice;
	VkDescriptorSetLayout m_DescriptorSetLayout;
	TextureManager& m_TextureManager;

	unordered_map<string, weak_ptr <const MaterialDescription> > m_Data;
};