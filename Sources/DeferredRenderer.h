#pragma once

#include "VulkanHelper.h"
#include "MaterialManager.h"
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
	DeferredRender(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent, VkFormat swapChainImageFormat, VkImageView shadowMapView, VkRenderPass dstRenderPass, shared_ptr<DescriptorSetLayout> materialDescriptorSetlayout);

private:
	void create1stPass();
	void create2ndPass();

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


	shared_ptr<DescriptorSet> m_2ndPassDescriptorSet;
	shared_ptr<const Sampler> m_Sampler;
	VkPipelineLayout m_2ndPassPipelineLayout;
	VkPipeline m_2ndPassPipeline;

	VkImageView m_ShadowMapView;

	unique_ptr<Buffer> m_lightParamBuffer; //TODO

	VkRenderPass m_DstRenderPass;
};
