#pragma once


#include "VulkanHelper.h"
#include "TextureManager.h"
#include <vector>
#include <array>
#include <memory>
#include <variant>
#include <tiny_obj_loader.h>
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

	Sampler(VkDevice device);
	~Sampler();
	Sampler& operator=(Sampler&& obj);
	Sampler(Sampler&& obj);

private:
	Sampler& operator=(Sampler const&) = default;
	Sampler(Sampler const&) = default;

	void destroy();
};

struct Buffer
{
	VkBuffer buffer;
	VkDeviceMemory bufferMemory;
	size_t size; //sizeof(T) * count
	size_t count;

	VkDevice m_Device = VK_NULL_HANDLE;

	template<typename T> explicit Buffer(VkPhysicalDevice physicalDevice, VkDevice device, T* bufferDataPtr,size_t  dataCnt, VkBufferUsageFlagBits usageBits)
	{
		m_Device = device;
		size = sizeof(T) * dataCnt;
		count = dataCnt;
		VulkanHelpers::createBuffer(buffer, bufferMemory, physicalDevice, device, bufferDataPtr, dataCnt, usageBits);
	}

	~Buffer();
	Buffer& operator=(Buffer&& obj);
	Buffer(Buffer&& obj);

	template<typename T> void updateBuffer(T* bufferDataPtr)
	{
		VulkanHelpers::copyData(bufferMemory, m_Device, bufferDataPtr, count);
	}
	
	template<typename T> void copyBuffer(T* bufferDataPtr) const
	{
		VulkanHelpers::readDataFromGPU(bufferMemory, m_Device, *bufferDataPtr, count);
	}

	void*  mapMemory() const
	{
		void* data;
		vkMapMemory(m_Device, bufferMemory, 0, size, 0, &data);
		return data;
	}

	void unmapMemory() const
	{
		vkUnmapMemory(m_Device, bufferMemory);
	}

private:
	Buffer& operator=(Buffer const&) = default;
	Buffer(Buffer const&) = default;

	void destroy();
};

struct ShaderParam
{
	VkDescriptorSetLayoutBinding m_DescriptorSetLayoutBinding;
	variant< VkDescriptorImageInfo, VkDescriptorBufferInfo> m_DescriptorInfo;
};

class DescriptorSetLayout
{
public:
	DescriptorSetLayout(VkDevice device);
	~DescriptorSetLayout();
	DescriptorSetLayout(DescriptorSetLayout const&) = delete;
	DescriptorSetLayout& operator=(DescriptorSetLayout const&) = delete;
	DescriptorSetLayout(DescriptorSetLayout &&) = delete;
	DescriptorSetLayout& operator=(DescriptorSetLayout &&) = delete;

	void addDescriptor(string const& paramName, size_t binding, VkShaderStageFlags shaderStages, VkDescriptorType type, unsigned int descriptorCount = 1);
	void createDescriptorSetLayout();
	VkDescriptorSetLayoutBinding const* getDescriptor(string const& paramName) const;
	size_t getDescriptorCount() const;

	template< typename Func> void enumerate(Func func)
	{
		for (auto& it : m_DescriptorSetLayoutBindingData)
		{
			func(it.second);
		}
	}

	VkDevice getDevice() const;
	VkDescriptorSetLayout getLayout() const;
private:
	VkDevice m_Device = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
	unordered_map<string, VkDescriptorSetLayoutBinding> m_DescriptorSetLayoutBindingData;
};

class DescriptorSet
{
public:
	DescriptorSet(shared_ptr<DescriptorSetLayout> descriptorSetLayout);
	~DescriptorSet();
	DescriptorSet(DescriptorSet&& obj);
	DescriptorSet& operator=(DescriptorSet&& obj);
	void createDescriptorSet();

	void setSamplerArray(string const& paramName, vector<VkImageView> const& imageViews, VkSampler sampler, VkImageLayout imageLayout);
	void setSampler(string const& paramName, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout);
	void setImageStorage(string const& paramName, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout);
	void setBuffer(string const& paramName, Buffer const& buffer);
	void setStorage(string const& paramName, Buffer const& buffer);

	VkDescriptorSet getDescriptorSet() const;
	shared_ptr<DescriptorSetLayout> getDescriptorSetlayout() const;
private:

	void updateDescriptorSet(VkDescriptorSetLayoutBinding const& descSetLayoutBinding);

	void destroy();
	DescriptorSet(DescriptorSet const&) = default;
	DescriptorSet& operator=(DescriptorSet const&) = default;

	VkDescriptorSet m_DescriptorSet;
	VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	shared_ptr<DescriptorSetLayout> m_DescriptorSetLayout;
	vector<pair< VkDescriptorType, variant< vector<VkDescriptorImageInfo>, VkDescriptorBufferInfo> >> m_DescriptorInfo;
};


struct MaterialDescription
{
	shared_ptr<const TextureData> m_DiffuseTexture;
	shared_ptr<const TextureData> m_SpecularTexture;
	shared_ptr<const TextureData> m_NormalTexture;
	shared_ptr<const TextureData> m_SpecularHighlightTexture;
	shared_ptr<const TextureData> m_AoTexture;
	shared_ptr<const Sampler> m_Sampler;
	
	DescriptorSet m_ShaderParams;
	Buffer m_UniformBuffer;

	string m_Id;
};

class MaterialManager
{
public:
	MaterialManager(TextureManager& textureManager, VkDevice device, VkPhysicalDevice physicalDevice);
	shared_ptr<const MaterialDescription> createMaterial(tinyobj::material_t const& material, const string& path);
	shared_ptr <DescriptorSetLayout> getDescriptorSetLayout() const;
	
private:
	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice;
	
	shared_ptr<const Sampler> m_Sampler;
	TextureManager& m_TextureManager;

	shared_ptr <DescriptorSetLayout> m_DecriptorSetLayout;

	unordered_map<string, weak_ptr <const MaterialDescription> > m_Data;
};