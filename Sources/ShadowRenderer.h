#pragma once

#include "VulkanHelper.h"
#include "MaterialManager.h"
#include <memory>


class ShadowRenderer
{

public:
	ShadowRenderer(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent);

public:
	VkExtent2D m_Extent;
	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice;
	VkFramebuffer m_ShadowFramebuffer;

	VkRenderPass m_ShadowRenderPass;
	AttachmentData m_ShadowAttachment;
	shared_ptr<DescriptorSet> m_ShadowDescriptorSet;

	VkPipelineLayout m_ShadowPipelineLayout;
	VkPipeline m_ShadowGraphicPipeline;

};
