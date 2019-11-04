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
	size_t size;

	VkDevice m_Device = VK_NULL_HANDLE;

	template<typename T> explicit Buffer(VkPhysicalDevice physicalDevice, VkDevice device, T* bufferDataPtr,size_t  dataCnt, VkBufferUsageFlagBits usageBits)
	{
		m_Device = device;
		size = sizeof(T) * dataCnt;
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

/*
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
*/

struct ShaderParam
{
	VkDescriptorSetLayoutBinding m_DescriptorSetLayoutBinding;
	variant< VkDescriptorImageInfo, VkDescriptorBufferInfo> m_DescriptorInfo;
};

/*
//TODO tohle by se melo asi samo vytvorit (create...)
struct DescriptorSetLayout
{
	DescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
	{
		m_Device = device;
		m_DescriptorSetLayout = descriptorSetLayout;
	}

	~DescriptorSetLayout()
	{
		destroy();
	}

	DescriptorSetLayout& operator=(DescriptorSetLayout&& obj)
	{
		destroy();
		*this = obj;
		obj.m_Device = VK_NULL_HANDLE;

		return *this;
	}

	DescriptorSetLayout(DescriptorSetLayout&& obj)
	{
		*this = move(obj);
	}

	VkDescriptorSetLayout get() const
	{
		return m_DescriptorSetLayout;
	}

private:
	DescriptorSetLayout& operator=(DescriptorSetLayout const&) = default;
	DescriptorSetLayout(DescriptorSetLayout const&) = default;

	void destroy()
	{
		if (m_Device == VK_NULL_HANDLE) return;

		vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
		m_Device = VK_NULL_HANDLE;
	}

	VkDevice m_Device = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_DescriptorSetLayout;
};
*/

class DescriptorSetLayout
{
public:
	DescriptorSetLayout(VkDevice device) : m_Device(device)
	{
	}

	~DescriptorSetLayout()
	{
		if (m_DescriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
			m_DescriptorSetLayout = VK_NULL_HANDLE;
		}
	}

	DescriptorSetLayout(DescriptorSetLayout const&) = delete;
	DescriptorSetLayout& operator=(DescriptorSetLayout const&) = delete;
	DescriptorSetLayout(DescriptorSetLayout &&) = delete;
	DescriptorSetLayout& operator=(DescriptorSetLayout &&) = delete;

	void addDescriptor(string const& paramName, size_t binding, VkShaderStageFlags shaderStages, VkDescriptorType type)
	{
		assert(m_DescriptorSetLayoutBindingData.count(paramName) == 0);

		auto& samplerLayoutBinding = m_DescriptorSetLayoutBindingData[paramName];
		samplerLayoutBinding = {};
		samplerLayoutBinding.binding = binding;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = type;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = shaderStages;
	}

	void createDescriptorSetLayout()
	{
		assert(m_DescriptorSetLayout == nullptr);

		vector< VkDescriptorSetLayoutBinding> bindings;
		for (auto &it : m_DescriptorSetLayoutBindingData) bindings.push_back(it.second);

		{	//check if binding ids are unique
			unordered_set<size_t> bindingIds;
			for_each(begin(bindings), end(bindings), [&bindings, &bindingIds](VkDescriptorSetLayoutBinding const& item)
				{
					assert(item.binding >= 0 && item.binding < bindings.size());
					auto it = bindingIds.insert(item.binding);
					assert(it.second);
				});
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = bindings.size();
		layoutInfo.pBindings = &bindings[0];

		auto res = vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout);
		assert(res == VK_SUCCESS);
	}

	VkDescriptorSetLayoutBinding const* getDescriptor(string const& paramName) const
	{
		auto it= m_DescriptorSetLayoutBindingData.find(paramName);
		if (it == m_DescriptorSetLayoutBindingData.end()) return nullptr;
		return &(it->second);
	}

	size_t getDescriptorCount() const
	{
		return m_DescriptorSetLayoutBindingData.size();
	}

	template< typename Func> void enumerate(Func func)
	{
		for (auto& it : m_DescriptorSetLayoutBindingData)
		{
			func(it.second);
		}
	}
	VkDevice getDevice() const
	{
		return m_Device;
	}

	VkDescriptorSetLayout getLayout() const
	{
		return m_DescriptorSetLayout;
	}

private:
	VkDevice m_Device = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
	unordered_map<string, VkDescriptorSetLayoutBinding> m_DescriptorSetLayoutBindingData;
};


class DescriptorSet
{
public:
	DescriptorSet(shared_ptr<DescriptorSetLayout> descriptorSetLayout)
	{
		m_DescriptorSetLayout = move(descriptorSetLayout);
	}

	~DescriptorSet()
	{
		destroy();
	}

	DescriptorSet(DescriptorSet &&obj)
	{
		*this = move(obj);
	}

	DescriptorSet& operator=(DescriptorSet &&obj)
	{
		*this = obj;
		obj.m_DescriptorPool = VK_NULL_HANDLE;

		return *this;
	}

	void createDescriptorSet()
	{
		unordered_multiset<VkDescriptorType> descriptorTypeCnt;

		for (auto& descInfo : m_DescriptorInfo)
		{
			if (get_if<VkDescriptorImageInfo>(&descInfo)) descriptorTypeCnt.insert(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			else if (get_if<VkDescriptorBufferInfo>(&descInfo)) descriptorTypeCnt.insert(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
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

		VkDevice device = m_DescriptorSetLayout->getDevice();

		auto res = vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool);
		assert(res == VK_SUCCESS);

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = 1;

		auto layout = m_DescriptorSetLayout->getLayout();
		allocInfo.pSetLayouts = &layout;

		res = vkAllocateDescriptorSets(device, &allocInfo, &m_DescriptorSet);

		assert(res == VK_SUCCESS);

		vector< VkWriteDescriptorSet> descriptorWriteSet;

		m_DescriptorSetLayout->enumerate([&descriptorWriteSet, this](VkDescriptorSetLayoutBinding const& descSetLayoutBinding)
			{
				VkWriteDescriptorSet newDescriptorSet = {};
				newDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				newDescriptorSet.dstSet = m_DescriptorSet;
				newDescriptorSet.dstBinding = descSetLayoutBinding.binding;
				newDescriptorSet.dstArrayElement = 0;
				newDescriptorSet.descriptorType = descSetLayoutBinding.descriptorType;
				newDescriptorSet.descriptorCount = descSetLayoutBinding.descriptorCount;
				newDescriptorSet.pImageInfo = get_if<VkDescriptorImageInfo>(&m_DescriptorInfo[descSetLayoutBinding.binding]);
				newDescriptorSet.pBufferInfo = get_if<VkDescriptorBufferInfo>(&m_DescriptorInfo[descSetLayoutBinding.binding]);

				descriptorWriteSet.push_back(newDescriptorSet);
			});

		vkUpdateDescriptorSets(device, descriptorWriteSet.size(), &descriptorWriteSet[0], 0, nullptr);
	}

	void addSampler(string const& paramName, VkImageView imageView, VkSampler sampler)
	{
		auto desc = m_DescriptorSetLayout->getDescriptor(paramName);
		assert(desc != nullptr && desc->descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = imageView;
		imageInfo.sampler = sampler;

		if (m_DescriptorInfo.size() <= desc->binding)  m_DescriptorInfo.resize(m_DescriptorSetLayout->getDescriptorCount());
		m_DescriptorInfo[desc->binding] = imageInfo;
	}

	void addBuffer(string const& paramName, Buffer const & buffer)
	{
		auto desc = m_DescriptorSetLayout->getDescriptor(paramName);
		assert(desc != nullptr && desc->descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = buffer.buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = buffer.size;

		if (m_DescriptorInfo.size() <= desc->binding)  m_DescriptorInfo.resize(m_DescriptorSetLayout->getDescriptorCount());
		m_DescriptorInfo[desc->binding] = bufferInfo;
	}

	VkDescriptorSet getDescriptorSet() const
	{
		return m_DescriptorSet;
	}

	shared_ptr<DescriptorSetLayout> getDescriptorSetlayout() const
	{
		return m_DescriptorSetLayout;
	}

private:

	void destroy()
	{
		if (m_DescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(m_DescriptorSetLayout->getDevice(), m_DescriptorPool, nullptr);
			m_DescriptorPool = VK_NULL_HANDLE;
		}
	}

	DescriptorSet(DescriptorSet const&) = default;
	DescriptorSet& operator=(DescriptorSet const&) = default;

	VkDescriptorSet m_DescriptorSet;
	VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	shared_ptr<DescriptorSetLayout> m_DescriptorSetLayout;
	vector<variant< VkDescriptorImageInfo, VkDescriptorBufferInfo>> m_DescriptorInfo;
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

	MaterialManager(TextureManager& textureManager, VkDevice device, VkPhysicalDevice physicalDevice, shared_ptr <DescriptorSetLayout> descriptorSetLayout) :m_TextureManager(textureManager)
	{
		m_Device = device;
		m_PhysicalDevice = physicalDevice;
		m_Sampler = shared_ptr<const Sampler>(new Sampler(m_Device));

		m_DecriptorSetLayout = move(descriptorSetLayout);
	}

	shared_ptr<const MaterialDescription> createMaterial(tinyobj::material_t const& material, const string& path)
	{
		string materialName = path+"/" + material.name;

		auto it = m_Data.find(materialName);
		if (it != end(m_Data))
		{
			return shared_ptr<const MaterialDescription>(it->second);
		}
		else
		{
			string defaultPath = path.substr(0, path.find("Models/")) + "Models/";

			auto diffuseTexture = !material.diffuse_texname.empty() ? m_TextureManager.loadTexture(path + "/" + material.diffuse_texname) : m_TextureManager.loadTexture(defaultPath + "/defaultDiffuse.png");
			auto specularTexture = !material.specular_texname.empty() ? m_TextureManager.loadTexture(path + "/" + material.specular_texname): m_TextureManager.loadTexture(defaultPath + "/defaultSpecular.png");
			auto specularHighlightTexture = !material.specular_highlight_texname.empty() ? m_TextureManager.loadTexture(path + "/" + material.specular_highlight_texname) : m_TextureManager.loadTexture(defaultPath + "/defaultSpecularExp.png");
			auto normalTexture = !material.specular_highlight_texname.empty() ? m_TextureManager.loadTexture(path + "/" + material.bump_texname) : m_TextureManager.loadTexture(defaultPath + "/defaultNormal.png");
			auto aoIt = material.unknown_parameter.find("map_Ao");
			auto aoTexture = (aoIt!=end(material.unknown_parameter)) ? m_TextureManager.loadTexture(path + "/" + aoIt->second) : m_TextureManager.loadTexture(defaultPath + "/defaultAo.png");

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
			
			DescriptorSet descriptorSet(m_DecriptorSetLayout);

			descriptorSet.addBuffer("materialBuffer",buffer);

			descriptorSet.addSampler("diffuseTex", diffuseTexture->imageView, m_Sampler->m_Sampler);
			descriptorSet.addSampler("specularTex", specularTexture->imageView, m_Sampler->m_Sampler);
			descriptorSet.addSampler("normalTex",normalTexture->imageView, m_Sampler->m_Sampler);
			descriptorSet.addSampler("specularHighlightTex", specularHighlightTexture->imageView, m_Sampler->m_Sampler);
			descriptorSet.addSampler("aoTex", aoTexture->imageView, m_Sampler->m_Sampler);
			descriptorSet.createDescriptorSet();

			MaterialDescription* newMaterial = new MaterialDescription{ move(diffuseTexture), move(specularTexture), move(normalTexture), move(specularHighlightTexture), move(aoTexture), m_Sampler, move(descriptorSet), move(buffer) };
			newMaterial->m_Id = materialName;

			shared_ptr<const MaterialDescription> sharedPtr(newMaterial, [this](MaterialDescription* matData)
				{
					//remove from database
					m_Data.erase(matData->m_Id);

					//free CPU memory
					delete matData;
				});

			//add into database
			m_Data[materialName] = sharedPtr;

			return sharedPtr;
		}
	}

	shared_ptr <DescriptorSetLayout> getDescriptorSetLayout() const
	{
		return m_DecriptorSetLayout;
	}
	
private:
	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice;
	
	shared_ptr<const Sampler> m_Sampler;
	TextureManager& m_TextureManager;

	shared_ptr <DescriptorSetLayout> m_DecriptorSetLayout;

	unordered_map<string, weak_ptr <const MaterialDescription> > m_Data;
};