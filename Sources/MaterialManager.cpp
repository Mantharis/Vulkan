#pragma once

#include "MaterialManager.h"
#include <unordered_map>
#include <unordered_set>
using namespace std;


	Sampler::Sampler(VkDevice device)
	{
		m_Device = device;
		VulkanHelpers::createTextureSampler(m_Sampler, m_Device);
	}

	Sampler::~Sampler()
	{
		destroy();
	}

	Sampler& Sampler::operator=(Sampler&& obj)
	{
		destroy();
		*this = obj;
		obj.m_Device = VK_NULL_HANDLE;

		return *this;
	}

	Sampler::Sampler(Sampler&& obj)
	{
		*this = move(obj);
	}

	void Sampler::destroy()
	{
		if (m_Device == VK_NULL_HANDLE) return;

		vkDestroySampler(m_Device, m_Sampler, nullptr);
		m_Device = VK_NULL_HANDLE;
	}



	Buffer::~Buffer()
	{
		destroy();
	}

	Buffer& Buffer::operator=(Buffer&& obj)
	{
		destroy();
		*this = obj;
		obj.m_Device = VK_NULL_HANDLE;

		return *this;
	}

	Buffer::Buffer(Buffer&& obj)
	{
		*this = move(obj);
	}

	void Buffer::destroy()
	{
		if (m_Device == VK_NULL_HANDLE) return;

		vkDestroyBuffer(m_Device, buffer, nullptr);
		vkFreeMemory(m_Device, bufferMemory, nullptr);
		m_Device = VK_NULL_HANDLE;
	}



	DescriptorSetLayout::DescriptorSetLayout(VkDevice device) : m_Device(device)
	{
	}

	DescriptorSetLayout::~DescriptorSetLayout()
	{
		if (m_DescriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
			m_DescriptorSetLayout = VK_NULL_HANDLE;
		}
	}

	
	void DescriptorSetLayout::addDescriptor(string const& paramName, size_t binding, VkShaderStageFlags shaderStages, VkDescriptorType type, unsigned int descriptorCount )
	{
		assert(m_DescriptorSetLayoutBindingData.count(paramName) == 0);

		auto& samplerLayoutBinding = m_DescriptorSetLayoutBindingData[paramName];
		samplerLayoutBinding = {};
		samplerLayoutBinding.binding = static_cast<uint32_t>(binding);
		samplerLayoutBinding.descriptorCount = descriptorCount;
		samplerLayoutBinding.descriptorType = type;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = shaderStages;
	}

	void DescriptorSetLayout::createDescriptorSetLayout()
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

		if (!bindings.empty())
		{
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = &bindings[0];
		}

		auto res = vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout);
		assert(res == VK_SUCCESS);
	}

	VkDescriptorSetLayoutBinding const* DescriptorSetLayout::getDescriptor(string const& paramName) const
	{
		auto it= m_DescriptorSetLayoutBindingData.find(paramName);
		if (it == m_DescriptorSetLayoutBindingData.end()) return nullptr;
		return &(it->second);
	}

	size_t DescriptorSetLayout::getDescriptorCount() const
	{
		return m_DescriptorSetLayoutBindingData.size();
	}

	
	VkDevice DescriptorSetLayout::getDevice() const
	{
		return m_Device;
	}

	VkDescriptorSetLayout DescriptorSetLayout::getLayout() const
	{
		return m_DescriptorSetLayout;
	}



	DescriptorSet::DescriptorSet(shared_ptr<DescriptorSetLayout> descriptorSetLayout)
	{
		m_DescriptorSetLayout = move(descriptorSetLayout);
	}

	DescriptorSet::~DescriptorSet()
	{
		destroy();
	}

	DescriptorSet::DescriptorSet(DescriptorSet &&obj)
	{
		*this = move(obj);
	}

	DescriptorSet& DescriptorSet::operator=(DescriptorSet &&obj)
	{
		*this = obj;
		obj.m_DescriptorPool = VK_NULL_HANDLE;

		return *this;
	}

	void DescriptorSet::updateDescriptorSet(VkDescriptorSetLayoutBinding const & descSetLayoutBinding)
	{
		VkWriteDescriptorSet newDescriptorSet = {};
		newDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		newDescriptorSet.dstSet = m_DescriptorSet;
		newDescriptorSet.dstBinding = descSetLayoutBinding.binding;
		newDescriptorSet.dstArrayElement = 0;
		newDescriptorSet.descriptorType = descSetLayoutBinding.descriptorType;
		newDescriptorSet.descriptorCount = descSetLayoutBinding.descriptorCount;

		auto descriptorImageInfoArray = get_if<vector<VkDescriptorImageInfo>>(&m_DescriptorInfo[descSetLayoutBinding.binding].second);
		newDescriptorSet.pImageInfo = descriptorImageInfoArray ? &descriptorImageInfoArray->at(0) : nullptr;
		newDescriptorSet.pBufferInfo = get_if<VkDescriptorBufferInfo>(&m_DescriptorInfo[descSetLayoutBinding.binding].second);

		vkUpdateDescriptorSets(m_DescriptorSetLayout->getDevice(), 1, &newDescriptorSet, 0, nullptr);
	}


	void DescriptorSet::createDescriptorSet()
	{
		unordered_multiset<VkDescriptorType> descriptorTypeCnt;

		m_DescriptorSetLayout->enumerate([&descriptorTypeCnt](VkDescriptorSetLayoutBinding& layoutBinding)
			{
				for (int i = 0; i < layoutBinding.descriptorCount; ++i)
				{
					//insert  layoutBinding.descriptorCount- times (this is really stupid way how to do it, but i feel lazy to refactor it at this moment)
					descriptorTypeCnt.insert(layoutBinding.descriptorType);
				}
				
			});

		vector< VkDescriptorPoolSize> poolSizes;
		for (auto& descriptorType : descriptorTypeCnt)
		{
			VkDescriptorPoolSize descriptorPoolSize;
			descriptorPoolSize.type = descriptorType;
			descriptorPoolSize.descriptorCount = static_cast<uint32_t>(descriptorTypeCnt.count(descriptorType));

			poolSizes.push_back(descriptorPoolSize);
		}

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
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
	}

	void DescriptorSet::setSamplerArray(string const& paramName, vector<VkImageView> const &imageViews, VkSampler sampler, VkImageLayout imageLayout)
	{
		auto desc = m_DescriptorSetLayout->getDescriptor(paramName);
		assert(desc != nullptr && desc->descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		vector<VkDescriptorImageInfo> imageInfos;

		for (auto imageView : imageViews)
		{
			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = imageLayout;
			imageInfo.imageView = imageView;
			imageInfo.sampler = sampler;


			imageInfos.push_back(imageInfo);
		}
		
		if (m_DescriptorInfo.size() <= desc->binding)  m_DescriptorInfo.resize(m_DescriptorSetLayout->getDescriptorCount());
		m_DescriptorInfo[desc->binding] = make_pair(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfos);

		updateDescriptorSet(*desc);
	}

	void DescriptorSet::setSampler(string const& paramName, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout)
	{
		setSamplerArray(paramName, { imageView }, sampler, imageLayout);
	}

	void DescriptorSet::setImageStorage(string const& paramName, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout)
	{
		auto desc = m_DescriptorSetLayout->getDescriptor(paramName);
		assert(desc != nullptr && desc->descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = imageLayout;
		imageInfo.imageView = imageView;
		imageInfo.sampler = sampler;

		if (m_DescriptorInfo.size() <= desc->binding)  m_DescriptorInfo.resize(m_DescriptorSetLayout->getDescriptorCount());
		m_DescriptorInfo[desc->binding] = make_pair(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, vector<VkDescriptorImageInfo>({ imageInfo }));

		updateDescriptorSet(*desc);
	}

	void DescriptorSet::setBuffer(string const& paramName, Buffer const & buffer)
	{
		auto desc = m_DescriptorSetLayout->getDescriptor(paramName);
		assert(desc != nullptr && desc->descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = buffer.buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = buffer.size;

		if (m_DescriptorInfo.size() <= desc->binding)  m_DescriptorInfo.resize(m_DescriptorSetLayout->getDescriptorCount());
		m_DescriptorInfo[desc->binding] = make_pair(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, bufferInfo);

		updateDescriptorSet(*desc);
	}

	void DescriptorSet::setStorage(string const& paramName, Buffer const& buffer)
	{
		auto desc = m_DescriptorSetLayout->getDescriptor(paramName);
		assert(desc != nullptr && desc->descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = buffer.buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = buffer.size;

		if (m_DescriptorInfo.size() <= desc->binding)  m_DescriptorInfo.resize(m_DescriptorSetLayout->getDescriptorCount());
		m_DescriptorInfo[desc->binding] = make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, bufferInfo);

		updateDescriptorSet(*desc);
	}


	VkDescriptorSet DescriptorSet::getDescriptorSet() const
	{
		return m_DescriptorSet;
	}

	shared_ptr<DescriptorSetLayout> DescriptorSet::getDescriptorSetlayout() const
	{
		return m_DescriptorSetLayout;
	}

	void DescriptorSet::destroy()
	{
		if (m_DescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(m_DescriptorSetLayout->getDevice(), m_DescriptorPool, nullptr);
			m_DescriptorPool = VK_NULL_HANDLE;
		}
	}


	MaterialManager::MaterialManager(TextureManager& textureManager, VkDevice device, VkPhysicalDevice physicalDevice) :m_TextureManager(textureManager)
	{
		m_Device = device;
		m_PhysicalDevice = physicalDevice;
		m_Sampler = shared_ptr<const Sampler>(new Sampler(m_Device));

		m_DecriptorSetLayout = make_shared<DescriptorSetLayout>(m_Device);
		m_DecriptorSetLayout->addDescriptor("materialBuffer", 0, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		m_DecriptorSetLayout->addDescriptor("diffuseTex", 1, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_DecriptorSetLayout->addDescriptor("specularTex", 2, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_DecriptorSetLayout->addDescriptor("normalTex", 3, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_DecriptorSetLayout->addDescriptor("specularHighlightTex", 4, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_DecriptorSetLayout->addDescriptor("aoTex", 5, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_DecriptorSetLayout->createDescriptorSetLayout();

	}

	shared_ptr<const MaterialDescription> MaterialManager::createMaterial(tinyobj::material_t const& material, const string& path)
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
			descriptorSet.createDescriptorSet();

			descriptorSet.setBuffer("materialBuffer",buffer);
			descriptorSet.setSampler("diffuseTex", diffuseTexture->imageView, m_Sampler->m_Sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			descriptorSet.setSampler("specularTex", specularTexture->imageView, m_Sampler->m_Sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			descriptorSet.setSampler("normalTex",normalTexture->imageView, m_Sampler->m_Sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			descriptorSet.setSampler("specularHighlightTex", specularHighlightTexture->imageView, m_Sampler->m_Sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			descriptorSet.setSampler("aoTex", aoTexture->imageView, m_Sampler->m_Sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			

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

	shared_ptr <DescriptorSetLayout> MaterialManager::getDescriptorSetLayout() const
	{
		return m_DecriptorSetLayout;
	}
	