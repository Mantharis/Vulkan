#pragma once

#include "VulkanHelper.h"
#include <assert.h>
#include <optional>
#include <fstream>
#include <vector>
#include <array>

#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace std;

	float VulkanHelpers::deg2Rad(float rad)
	{
		return rad * 2.0f * Pi / 360.0f;
	}

	glm::mat4 VulkanHelpers::preparePerspectiveProjectionMatrix(float aspect_ratio, float field_of_view, float near_plane, float far_plane)
	{
		float f = 1.0f / tan(deg2Rad(0.5f * field_of_view));

		glm::mat4 projectionMatrix;
		projectionMatrix[0] = glm::vec4(f / aspect_ratio, 0.0f, 0.0f, 0.0f);
		projectionMatrix[1] = glm::vec4(0.0f, -f, 0.0f, 0.0f);
		projectionMatrix[2] = glm::vec4(0.0f, 0.0f, far_plane / (near_plane - far_plane), -1.0f);
		projectionMatrix[3] = glm::vec4(f / aspect_ratio, 0.0f, (near_plane * far_plane) / (near_plane - far_plane), 0.0f);

		return projectionMatrix;
	}

	void VulkanHelpers::createAttachmnent(AttachmentData &outAttachmentData, VkPhysicalDevice physicalDevice, VkDevice device, VkFormat imageFormat, VkImageUsageFlags imageUsageFlags, VkImageAspectFlags aspectFlags , VkExtent2D extent)
	{
		//create color attachment
		VulkanHelpers::createImage(physicalDevice, device, extent.width, extent.width, imageFormat,
			VK_IMAGE_TILING_OPTIMAL, imageUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			outAttachmentData.image, outAttachmentData.memory);

		VulkanHelpers::createImageView(outAttachmentData.view, device, outAttachmentData.image, imageFormat, aspectFlags);
	}

	void VulkanHelpers::createFramebuffer(VkFramebuffer &outFramebuffer, VkRenderPass renderPass,  vector<VkImageView>& attachmentData, VkDevice device,  VkExtent2D extent)
	{
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentData.size());
		framebufferInfo.pAttachments = &attachmentData[0];
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;

		auto res = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &outFramebuffer);
		assert(res == VK_SUCCESS);
	}

	

 void VulkanHelpers::createTextureSampler(VkSampler& textureSampler, VkDevice device)
	{
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		auto res = vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler);
		assert(res == VK_SUCCESS);
	}

	 void VulkanHelpers::createGraphicsPipeline(VkDevice device, VkRenderPass renderPass, int colorAttachmentCnt, VkExtent2D extent, ShaderSet const& shaderSet, bool blending, VkPipelineInputAssemblyStateCreateInfo const &inputAssemblyInfo, VkPipelineLayout& outPipelineLayout, VkPipeline& outPipeline)
	{
		vector< VkPipelineShaderStageCreateInfo> shaders;

		{
			VkShaderModule vertShader;
			VulkanHelpers::createShaderModuleFromFile(shaderSet.vertexShaderPath.c_str(), device, vertShader);
			VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = vertShader;
			vertShaderStageInfo.pName = "main";
			shaders.push_back(vertShaderStageInfo);
		}

		{
			VkShaderModule fragShader;
			VulkanHelpers::createShaderModuleFromFile(shaderSet.fragmentShaderPath.c_str(), device, fragShader);
			VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = fragShader;
			fragShaderStageInfo.pName = "main";
			shaders.push_back(fragShaderStageInfo);
		}
		
		if (!shaderSet.geometryShaderPath.empty())
		{
			VkShaderModule geomShader;
			VulkanHelpers::createShaderModuleFromFile(shaderSet.geometryShaderPath.c_str(), device, geomShader);
			VkPipelineShaderStageCreateInfo geomShaderStageInfo = {};
			geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			geomShaderStageInfo.module = geomShader;
			geomShaderStageInfo.pName = "main";
			shaders.push_back(geomShaderStageInfo);
		}

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		if (!shaderSet.vertexInputAttributeDescription.empty())
		{
			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.pVertexBindingDescriptions = &shaderSet.vertexInputBindingDescription;
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(shaderSet.vertexInputAttributeDescription.size());
			vertexInputInfo.pVertexAttributeDescriptions = &shaderSet.vertexInputAttributeDescription[0];
		}

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)extent.width;
		viewport.height = (float)extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = extent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		vector< VkPipelineColorBlendAttachmentState> colorBlendAttachments;

		for (size_t i = 0; i < colorAttachmentCnt; ++i)
		{
			VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachment.blendEnable = blending;
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

			colorBlendAttachments.push_back(colorBlendAttachment);
		}


		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional

		if (!colorBlendAttachments.empty())
		{
			colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
			colorBlending.pAttachments = &colorBlendAttachments[0];
		}
		
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		if (!shaderSet.descriptorSetLayout.empty())
		{
			pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(shaderSet.descriptorSetLayout.size());
			pipelineLayoutInfo.pSetLayouts = &shaderSet.descriptorSetLayout[0];
		}
	

		if (!shaderSet.pushConstant.empty())
		{
			pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(shaderSet.pushConstant.size());
			pipelineLayoutInfo.pPushConstantRanges = &shaderSet.pushConstant[0];
		}


		auto res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &outPipelineLayout);
		assert(VK_SUCCESS == res);

		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {}; // Optional
		depthStencil.back = {}; // Optional



		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = shaders.size();
		pipelineInfo.pStages = &shaders[0];
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; // Optional
		pipelineInfo.layout = outPipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &outPipeline);
		assert(res == VK_SUCCESS);

		for (auto& shader : shaders)
		{
			vkDestroyShaderModule(device, shader.module, nullptr);
		}
	}

	void VulkanHelpers::createRenderPass(VkRenderPass &outRenderPass, VkDevice device, VkFormat colorAttachmentFormat, VkImageLayout colroAttachmnetImagelayout, VkAttachmentLoadOp colorAttachmentLoadOp, size_t colorAttachmentCnt,  bool depthAttachment, VkImageLayout depthAttachmentImageLayout)
	{
		vector< VkAttachmentDescription> attachmentDescriptions;
		vector< VkAttachmentReference > colorReferences;
		VkAttachmentReference depthReference = { static_cast<uint32_t>(colorAttachmentCnt), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };


		for (size_t i = 0; i < colorAttachmentCnt; ++i)
		{
			VkAttachmentDescription attachmentDescription = {};
			attachmentDescription.format = colorAttachmentFormat;
			attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			attachmentDescription.loadOp = colorAttachmentLoadOp; // VK_ATTACHMENT_LOAD_OP_LOAD; // VK_ATTACHMENT_LOAD_OP_CLEAR; //VK_ATTACHMENT_LOAD_OP_LOAD
			attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescription.finalLayout = colroAttachmnetImagelayout;

			attachmentDescriptions.push_back(attachmentDescription);

			VkAttachmentReference colorReference = { static_cast<uint32_t>(i), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
			colorReferences.push_back(colorReference);
		}

		if (depthAttachment)
		{
			VkAttachmentDescription attchmentDescription = {};
			attchmentDescription.format = VK_FORMAT_D16_UNORM;
			attchmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			attchmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attchmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attchmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attchmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attchmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attchmentDescription.finalLayout = depthAttachmentImageLayout;

			attachmentDescriptions.push_back(attchmentDescription);
		}



		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		if (!colorReferences.empty())
		{
			subpassDescription.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
			subpassDescription.pColorAttachments = &colorReferences[0];
		}
		
		if (depthAttachment) subpassDescription.pDepthStencilAttachment = &depthReference;
		
		/*
		// Use subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 1> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		*/

		// Use subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		//if (!colorReferences.empty()) dependencies[0].dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		//if (!colorReferences.empty()) dependencies[0].dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// Create the actual renderpass
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
		renderPassInfo.pAttachments = attachmentDescriptions.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();


		auto res = vkCreateRenderPass(device, &renderPassInfo, nullptr, &outRenderPass);

		assert(res == VK_SUCCESS);

	}


	uint32_t VulkanHelpers::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) &&
				(memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		assert(!"Type not found!");
		return 0;
	}

	VkCommandBuffer VulkanHelpers::beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void VulkanHelpers::endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void VulkanHelpers::createImageView(VkImageView& outImageView, VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectMask)
	{
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectMask;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		auto res = vkCreateImageView(device, &viewInfo, nullptr, &outImageView);
		assert(res == VK_SUCCESS);
	}


	void VulkanHelpers::createTextureImage(VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicQueue, VkImage& outImage, VkDeviceMemory& outDeviceMemory, const char* imagePath, VkDevice device, VkFormat imageFormat )
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(imagePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		assert(pixels != nullptr);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		createBuffer(stagingBuffer, stagingBufferMemory, physicalDevice, device, pixels, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		stbi_image_free(pixels);

		//VK_FORMAT_R8G8B8A8_UNORM
		createImage(physicalDevice, device, texWidth, texHeight, imageFormat , VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outImage, outDeviceMemory);
		transitionImageLayout(device, commandPool, graphicQueue, outImage, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(device, commandPool, graphicQueue, stagingBuffer, outImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		transitionImageLayout(device, commandPool, graphicQueue, outImage, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void VulkanHelpers::createImage(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
	{
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		auto res = vkCreateImage(device, &imageInfo, nullptr, &image);
		assert(res == VK_SUCCESS);

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;

		allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

		res = vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory);
		assert(res == VK_SUCCESS);

		vkBindImageMemory(device, image, imageMemory, 0);
	}

	void VulkanHelpers::writeImage(const char *filePath, size_t width, size_t height, size_t channels, void const *data)
	{
			stbi_write_jpg(filePath, width, height, channels, data, width * channels);
	}


	void VulkanHelpers::createShaderModuleFromFile(const char* filePath, VkDevice device, VkShaderModule& outShaderModule)
	{
		ifstream file(filePath, ifstream::binary);

		assert(file);

		file.seekg(0, file.end);
		size_t size = file.tellg();
		string buffer;
		buffer.resize(size);

		file.seekg(0, file.beg);
		file.read(&buffer[0], size);
		file.close();

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = buffer.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

		auto res = vkCreateShaderModule(device, &createInfo, nullptr, &outShaderModule);
		assert(res == VK_SUCCESS);
	}

	void VulkanHelpers::transitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = 0;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
		{
			assert(!"unsupported layout transition!");
		}

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		endSingleTimeCommands(device, commandPool, graphicQueue, commandBuffer);
	}

	void VulkanHelpers::copyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue graphicQueue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		endSingleTimeCommands(device, commandPool, graphicQueue, commandBuffer);
	}


float const VulkanHelpers::Pi = 3.14159265358979323846f;