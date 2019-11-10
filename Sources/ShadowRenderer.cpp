#pragma once

#include "ShadowRenderer.h"
#include "VulkanHelper.h"
#include "VertexFormat.h"
#include <glm/mat4x4.hpp>


using namespace std;


ShadowRenderer::ShadowRenderer(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent) : m_Device(device), m_PhysicalDevice(physicalDevice), m_Extent(extent)
{
	VulkanHelpers::createRenderPass(m_ShadowRenderPass, m_Device, VkFormat::VK_FORMAT_END_RANGE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, 0, true, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
	VulkanHelpers::createAttachmnent(m_ShadowAttachment, m_PhysicalDevice, m_Device, VK_FORMAT_D16_UNORM, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, m_Extent);

	vector<VkImageView> attachments = { m_ShadowAttachment.view };

	VulkanHelpers::createFramebuffer(m_ShadowFramebuffer, m_ShadowRenderPass, attachments, m_Device, m_Extent);

	//DESCRIPTOR SET
	shared_ptr<DescriptorSetLayout> descriptorSetLayout = make_shared< DescriptorSetLayout>(m_Device);
	descriptorSetLayout->createDescriptorSetLayout();

	m_ShadowDescriptorSet = make_shared< DescriptorSet>(descriptorSetLayout);
	m_ShadowDescriptorSet->createDescriptorSet();

	VkDescriptorSet shadowDecriptorSet = m_ShadowDescriptorSet->getDescriptorSet();

	//GRAPHIC PIPELINE
	ShaderSet shaderData;
	shaderData.vertexInputBindingDescription = Vertex::getBindingDescription();
	shaderData.vertexShaderPath = string("./../Shaders/shadowShaderVert.spv");
	shaderData.fragmentShaderPath = string("./../Shaders/shadowShaderFrag.spv");
	shaderData.pushConstant.resize(1);
	shaderData.pushConstant[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	shaderData.pushConstant[0].offset = 0;
	shaderData.pushConstant[0].size = sizeof(glm::mat4) * 3;
	shaderData.descriptorSetLayout.push_back(m_ShadowDescriptorSet->getDescriptorSetlayout()->getLayout());

	for (auto& attribute : Vertex::getAttributeDescriptions())
		shaderData.vertexInputAttributeDescription.push_back(attribute);
	
	VulkanHelpers::createGraphicsPipeline(m_Device, m_ShadowRenderPass, 0, m_Extent, shaderData, false, m_ShadowPipelineLayout, m_ShadowGraphicPipeline);
}


