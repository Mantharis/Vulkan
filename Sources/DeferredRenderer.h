#pragma once

#include "VulkanHelper.h"
#include <memory>

using namespace std;


struct LightParamUBO
{
	alignas(16) glm::mat4 viewProj;
	alignas(16) glm::vec3 viewSpacePos;
	alignas(16) glm::vec3 color;
};


class DeferredRender
{

public:
	DeferredRender(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent, VkFormat swapChainImageFormat) : m_Device(device), m_PhysicalDevice(physicalDevice), m_Extent(extent), m_SwapChainImageFormat(swapChainImageFormat)
	{
		create1stPass();
		create2ndPass();

	}


	static VkImageView depth;

private:

	void create1stPass()
	{
		m_1stPassDescriptorSetLayout = make_shared<DescriptorSetLayout>(m_Device);
		m_1stPassDescriptorSetLayout->addDescriptor("materialBuffer", 0, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		m_1stPassDescriptorSetLayout->addDescriptor("diffuseTex", 1, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_1stPassDescriptorSetLayout->addDescriptor("specularTex", 2, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_1stPassDescriptorSetLayout->addDescriptor("normalTex", 3, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_1stPassDescriptorSetLayout->addDescriptor("specularHighlightTex", 4, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_1stPassDescriptorSetLayout->addDescriptor("aoTex", 5, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_1stPassDescriptorSetLayout->createDescriptorSetLayout();

		//RENDER PASS
		VulkanHelpers::createRenderPass(m_1stRenderPass, m_Device, s_ImageFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, s_ColorAttachmentCnt, true, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

		

		//COLOR ATTACHMENTS
		m_1stColorAttachments.resize(s_ColorAttachmentCnt);

		for (auto& it : m_1stColorAttachments)
			VulkanHelpers::createAttachmnent(it, m_PhysicalDevice, m_Device, s_ImageFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, m_Extent);

		//DEPTH ATTACHMENT
		VulkanHelpers::createAttachmnent(m_1stDepthAttachment, m_PhysicalDevice, m_Device, VK_FORMAT_D16_UNORM, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, m_Extent);

		//FRAMEBUFFER
		vector<VkImageView> attachments;

		for (auto& it : m_1stColorAttachments)
			attachments.push_back(it.view);

		attachments.push_back(m_1stDepthAttachment.view);

		VulkanHelpers::createFramebuffer(m_1stFramebuffer, m_1stRenderPass, attachments, m_Device, m_Extent);


		m_1stPassDescriptorSetLayout2 = make_shared<DescriptorSetLayout>(m_Device);
		m_1stPassDescriptorSetLayout2->addDescriptor("sceneBuffer", 0, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		m_1stPassDescriptorSetLayout2->createDescriptorSetLayout();


		ShaderSet shaderData;

		for (auto& attribute : Vertex::getAttributeDescriptions())
			shaderData.vertexInputAttributeDescription.push_back(attribute);

		shaderData.vertexInputBindingDescription = Vertex::getBindingDescription();
		shaderData.vertexShaderPath = string("./../Shaders/vert.spv");
		shaderData.fragmentShaderPath = string("./../Shaders/defferedShader1stPass.spv");
		shaderData.pushConstant.resize(1);
		shaderData.pushConstant[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		shaderData.pushConstant[0].offset = 0;
		shaderData.pushConstant[0].size = sizeof(glm::mat4) * 3;
		shaderData.descriptorSetLayout.push_back(m_1stPassDescriptorSetLayout->getLayout());
		shaderData.descriptorSetLayout.push_back(m_1stPassDescriptorSetLayout2->getLayout());

		VulkanHelpers::createGraphicsPipeline(m_Device, m_1stRenderPass, 4, m_Extent, shaderData, m_1stPassPipelineLayout, m_1stPassPipeline);

	}


	void create2ndPass()
	{
		VulkanHelpers::createRenderPass(m_2ndRenderPass, m_Device, m_SwapChainImageFormat, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1, true, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		//DESCRIPTOR SET
		shared_ptr<DescriptorSetLayout> descriptorSetLayout = make_shared< DescriptorSetLayout>(m_Device);
		descriptorSetLayout->addDescriptor("metalicRoughnessAo", 3, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorSetLayout->addDescriptor("normal", 2, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorSetLayout->addDescriptor("albedo", 1, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorSetLayout->addDescriptor("pos", 0, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorSetLayout->addDescriptor("depth", 4, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorSetLayout->addDescriptor("lightParams", 5, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		descriptorSetLayout->createDescriptorSetLayout();

		m_2ndPassDescriptorSet = make_shared< DescriptorSet>(descriptorSetLayout);

		m_Sampler = shared_ptr<const Sampler>(new Sampler(m_Device));

		m_2ndPassDescriptorSet->addSampler("albedo", m_1stColorAttachments[0].view, m_Sampler->m_Sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_2ndPassDescriptorSet->addSampler("pos", m_1stColorAttachments[1].view, m_Sampler->m_Sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_2ndPassDescriptorSet->addSampler("normal", m_1stColorAttachments[2].view, m_Sampler->m_Sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_2ndPassDescriptorSet->addSampler("metalicRoughnessAo", m_1stColorAttachments[3].view, m_Sampler->m_Sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		m_2ndPassDescriptorSet->addSampler("depth", depth /*m_1stDepthAttachment.view*/, m_Sampler->m_Sampler, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

		//==============================
		LightParamUBO lightParam;
		m_lightParamBuffer = make_unique<Buffer>(m_PhysicalDevice, m_Device, &lightParam, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		m_2ndPassDescriptorSet->addBuffer("lightParams", *m_lightParamBuffer);
		//==============================

		m_2ndPassDescriptorSet->createDescriptorSet();


		//GRAPHIC PIPELINE
		ShaderSet shaderData;
		shaderData.vertexInputBindingDescription = {};
		shaderData.vertexShaderPath = string("./../Shaders/defferedShader2ndPassVert.spv");
		shaderData.fragmentShaderPath = string("./../Shaders/defferedShader2ndPassFrag.spv");
		shaderData.descriptorSetLayout.push_back(m_2ndPassDescriptorSet->getDescriptorSetlayout()->getLayout());

		VulkanHelpers::createGraphicsPipeline(m_Device, m_2ndRenderPass, 1, m_Extent, shaderData, m_2ndPassPipelineLayout, m_2ndPassPipeline);
	}

	static const int s_ColorAttachmentCnt = 4;
	static const VkFormat s_ImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

public:
	VkExtent2D m_Extent;
	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice;
	VkFormat m_SwapChainImageFormat;

	shared_ptr<DescriptorSetLayout> m_1stPassDescriptorSetLayout;
	shared_ptr<DescriptorSetLayout> m_1stPassDescriptorSetLayout2;

	VkRenderPass m_1stRenderPass;
	VkFramebuffer m_1stFramebuffer;
	vector<AttachmentData> m_1stColorAttachments;
	AttachmentData m_1stDepthAttachment;
	VkPipelineLayout m_1stPassPipelineLayout;
	VkPipeline m_1stPassPipeline;


	VkRenderPass m_2ndRenderPass;

	shared_ptr<DescriptorSet> m_2ndPassDescriptorSet;
	shared_ptr<const Sampler> m_Sampler;
	VkPipelineLayout m_2ndPassPipelineLayout;
	VkPipeline m_2ndPassPipeline;

	unique_ptr<Buffer> m_lightParamBuffer; //TODO
};

VkImageView DeferredRender::depth = VK_NULL_HANDLE;