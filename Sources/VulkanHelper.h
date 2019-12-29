#pragma once

#include <vulkan/vulkan.h>
#include <optional>
#include <vector>
#include <string>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>

using namespace std;

struct AttachmentData
{
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
};

struct FramebufferData
{
	VkRenderPass renderPass;
	vector< AttachmentData> colorAttachments;
	optional< AttachmentData> depthAttachment;
	VkFramebuffer framebuffer;
};

struct ShaderSet
{
	vector<VkDescriptorSetLayout> descriptorSetLayout;
	vector< VkPushConstantRange> pushConstant;

	VkVertexInputBindingDescription vertexInputBindingDescription;
	vector< VkVertexInputAttributeDescription> vertexInputAttributeDescription;

	string vertexShaderPath;
	string fragmentShaderPath;
	string geometryShaderPath;
};


class VulkanHelpers
{
public:
	static float deg2Rad(float rad);
	static glm::mat4 preparePerspectiveProjectionMatrix(float aspect_ratio, float field_of_view, float near_plane, float far_plane);
	static void createAttachmnent(AttachmentData& outAttachmentData, VkPhysicalDevice physicalDevice, VkDevice device, VkFormat imageFormat, VkImageUsageFlags imageUsageFlags, VkImageAspectFlags aspectFlags, VkExtent2D extent);
	static void createFramebuffer(VkFramebuffer& outFramebuffer, VkRenderPass renderPass, vector<VkImageView>& attachmentData, VkDevice device, VkExtent2D extent);
	static void createTextureSampler(VkSampler& textureSampler, VkDevice device);
	static void createGraphicsPipeline(VkDevice device, VkRenderPass renderPass, int colorAttachmentCnt, VkExtent2D extent, ShaderSet const& shaderSet, bool blending, VkPipelineInputAssemblyStateCreateInfo const &inputAssembly,  VkPipelineLayout& outPipelineLayout, VkPipeline& outPipeline);
	static void createRenderPass(VkRenderPass& outRenderPass, VkDevice device, VkFormat colorAttachmentFormat, VkImageLayout colroAttachmnetImagelayout, VkAttachmentLoadOp colorAttachmentLoadOp, size_t colorAttachmentCnt, bool depthAttachment, VkImageLayout depthAttachmentImageLayout);
	static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	static VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
	static void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkCommandBuffer commandBuffer);
	static void createImageView(VkImageView& outImageView, VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectMask);
	static void createTextureImage(VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicQueue, VkImage& outImage, VkDeviceMemory& outDeviceMemory, const char* imagePath, VkDevice device, VkFormat imageFormat);
	static void createImage(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	static void createShaderModuleFromFile(const char* filePath, VkDevice device, VkShaderModule& outShaderModule);
	static void writeImage(const char* filePath, size_t width, size_t height, size_t channels, void const* data);

	template<typename T> static void createBuffer(VkBuffer& buffer, VkDeviceMemory& memory, VkPhysicalDevice physicalDevice, VkDevice device, T const* inputData, size_t inputSize, VkBufferUsageFlags bufferUsageFlag)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(T) * inputSize;
		bufferInfo.usage = bufferUsageFlag;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		auto res = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
		assert(VK_SUCCESS == res);

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		res = vkAllocateMemory(device, &allocInfo, nullptr, &memory);
		assert(VK_SUCCESS == res);

		vkBindBufferMemory(device, buffer, memory, 0);

		copyData(memory, device, inputData, inputSize);
	}

	template<typename T> static void copyData(VkDeviceMemory& memory, VkDevice device, T const* inputData, size_t inputSize)
	{
		void* data;
		vkMapMemory(device, memory, 0, sizeof(T) * inputSize, 0, &data);
		memcpy(data, inputData, sizeof(T) * inputSize);
		vkUnmapMemory(device, memory);
	}

	template<typename T> static void readDataFromGPU(VkDeviceMemory memory, VkDevice device, T& dstBuffer, size_t inputSize)
	{
		void* data;
		vkMapMemory(device, memory, 0, sizeof(T) * inputSize, 0, &data);
		memcpy(&dstBuffer, data, sizeof(T) * inputSize);
		vkUnmapMemory(device, memory);
	}

	static void transitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

private:
	
	static void copyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue graphicQueue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	static float const Pi;
};
