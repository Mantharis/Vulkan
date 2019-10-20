#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <glm/gtc/matrix_transform.hpp>


#include <vector>
#include <math.h>
#include <iostream>
#include <fstream>
#include <array>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

#include "VulkanHelper.h"
#include "Model.h"

#include "TextureManager.h"
#include "ModelManager.h"
#include "SceneObjectManager.h"
#include "Camera.h"
#include "VisualComponent.h"

#include "MaterialManager.h"

 using namespace std;

 float const Pi = 3.14159265358979323846f;

 float deg2Rad(float rad)
 {
	 return rad * 2.0f * Pi / 360.0f;
 }

 glm::mat4 preparePerspectiveProjectionMatrix(float aspect_ratio, float field_of_view, float near_plane, float far_plane) 
 {
	 float f = 1.0f / tan(deg2Rad(0.5f * field_of_view));

	 glm::mat4 projectionMatrix;
	 projectionMatrix[0] = glm::vec4(f / aspect_ratio, 0.0f, 0.0f, 0.0f);
	 projectionMatrix[1] = glm::vec4(0.0f, -f, 0.0f, 0.0f);
	 projectionMatrix[2] = glm::vec4(0.0f, 0.0f, far_plane / (near_plane - far_plane), -1.0f);
	 projectionMatrix[3] = glm::vec4(f / aspect_ratio, 0.0f, (near_plane * far_plane) / (near_plane - far_plane), 0.0f);

	 return projectionMatrix;
 }

class Window
{
public:
	Window(unsigned int width, unsigned int height, const char* title)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	}

	bool shouldClose()
	{
		return glfwWindowShouldClose(m_Window);
	}

	void pollEvents()
	{
		glfwPollEvents();
	}

	GLFWwindow& getWindow()
	{
		assert(m_Window != nullptr);
		return *m_Window;
	}

	~Window()
	{
		glfwDestroyWindow(m_Window);
		glfwTerminate();
		m_Window = nullptr;
	}
private:
	GLFWwindow* m_Window = nullptr;
};


class SceneObjectFactory
{
public:
	SceneObjectFactory(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool  commandPool, VkQueue graphicQueue, VkDescriptorSetLayout descriptorSetLayout)
	{
		m_TextureManager = make_unique<TextureManager>(physicalDevice, device, commandPool, graphicQueue);
		m_MaterialManager = make_unique<MaterialManager>(*m_TextureManager, device, physicalDevice, descriptorSetLayout);
		m_ModelManager = make_unique<ModelManager>(physicalDevice, device, *m_MaterialManager);
	}

	unique_ptr<SceneObject> createSceneObjectFromFile(string const& path)
	{
		auto cityModel = m_ModelManager->loadModel(path);

		unique_ptr<SceneObject> obj = make_unique<SceneObject>();
		unique_ptr<VisualComponent> visualComp = make_unique<VisualComponent>(cityModel);
		obj->addComponent(move(visualComp));

		return obj;
	}

private:
	unique_ptr<TextureManager> m_TextureManager;
	unique_ptr< ModelManager> m_ModelManager;
	unique_ptr<MaterialManager> m_MaterialManager;
};


const unsigned int MAX_LIGHT_COUNT = 10;
struct SceneData
{
	alignas(16) glm::vec3 cameraPos = glm::vec3(1.0f, 0.0f, 0.0f);
	alignas(16) glm::vec3 lightPos = glm::vec3(0.0f, 590.0f, 0.0f);
	alignas(16) glm::vec3 lightColor;
	alignas(4) unsigned int lightCount;
};

struct SceneDescription
{
	SceneData m_Data;
	Buffer m_UniformBuffer;
	DescriptorSet m_DescriptorSet;

	SceneDescription(VkPhysicalDevice physicalDevice, VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
		:m_UniformBuffer(physicalDevice, device, &m_Data, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),
		m_DescriptorSet(device, descriptorSetLayout, vector<variant< VkDescriptorImageInfo, VkDescriptorBufferInfo>>(1, VkDescriptorBufferInfo{ m_UniformBuffer.buffer, 0, sizeof(SceneData)}) )
	{
	}

};


class VulcanInstance
{
public:
	VulcanInstance(GLFWwindow &window)
	{
		m_Camera.setPos( glm::vec3(0.0f, 100.0f, 300.0f));
		m_Camera.setDir(glm::vec3(0.0f, 0.0f, -1.0f));

		m_ProjectionMatrix = preparePerspectiveProjectionMatrix(800.0f / 600.0f, 60, 1.0f, 10000.0f);



		createInstance();
		createSurface(window);

		const vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

		selectPhysicalDevice(deviceExtensions);

		createDevice(deviceExtensions);
		createSwapChain();
		createImageViews();
		createRenderPass();

		createDescriptorSetLayout();
		createGraphicsPipeline();
		createDepthResources();
		createFramebuffers();
		createCommandPool();

		m_SceneObjectFactory = make_unique<SceneObjectFactory>(m_PhysicalDevice, m_Device, m_CommandPool, m_GraphicQueue, m_MaterialDescriptorSetLayout);

		unique_ptr<SceneObject> obj = m_SceneObjectFactory->createSceneObjectFromFile("./../Models/house_01.obj");

		glm::mat4& mat = obj->getMatrix();
		mat[0][0] = mat[1][1] = mat[2][2] = mat[3][3] = 0.1f;
		mat[3][0] = 1.0f;

		m_Scene.insert(move(obj));


		m_SceneDescription = make_unique<SceneDescription>(m_PhysicalDevice, m_Device, m_SceneDescriptorSetLayout);

		createCommandBuffers();
		createSyncPrimitives();
	}

	~VulcanInstance()
	{
		vkDeviceWaitIdle(m_Device);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
		}
	
		m_Scene.clear();

		//TODO 
		//m_Models[0].destroy();
		//m_Models.clear();

		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

		for (auto framebuffer : m_SwapChainFramebuffers) vkDestroyFramebuffer(m_Device, framebuffer, nullptr);

		vkDestroyDescriptorSetLayout(m_Device, m_MaterialDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_Device, m_SceneDescriptorSetLayout, nullptr);

		vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
		vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

		for (auto imageView : m_SwapChainImageViews) vkDestroyImageView(m_Device, imageView, nullptr);

		vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
		vkDestroyDevice(m_Device, nullptr);
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
	}

	void drawFrame()
	{
		vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

		uint32_t imageIndex;
		auto res = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
		assert(res == VK_SUCCESS);

		recordCommandBuffer(m_CommandBuffers[imageIndex], m_SwapChainFramebuffers[imageIndex]);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &m_ImageAvailableSemaphores[m_CurrentFrame];
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrame];

		res = vkQueueSubmit(m_GraphicQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]);
		assert(res == VK_SUCCESS);


		//==========
		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrame];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_SwapChain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional

		res = vkQueuePresentKHR(m_PresentationQueue, &presentInfo);
		assert(res == VK_SUCCESS);

		m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

private:

	void createDepthResources()
	{
		VulkanHelpers::createImage(m_PhysicalDevice, m_Device, 800, 600, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);
		VulkanHelpers::createImageView(m_DepthImageView, m_Device, m_DepthImage, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	void createDescriptorSetLayout() 
	{
		{
			VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
			samplerLayoutBinding.binding = 1;
			samplerLayoutBinding.descriptorCount = 1;
			samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerLayoutBinding.pImmutableSamplers = nullptr;
			samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			VkDescriptorSetLayoutBinding samplerLayoutBinding2 = {};
			samplerLayoutBinding2.binding = 2;
			samplerLayoutBinding2.descriptorCount = 1;
			samplerLayoutBinding2.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerLayoutBinding2.pImmutableSamplers = nullptr;
			samplerLayoutBinding2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			VkDescriptorSetLayoutBinding uboLayoutBinding = {};
			uboLayoutBinding.binding = 0;
			uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboLayoutBinding.descriptorCount = 1;
			uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			uboLayoutBinding.pImmutableSamplers = nullptr; // Optional


			array<VkDescriptorSetLayoutBinding, 3> bindings = { uboLayoutBinding, samplerLayoutBinding, samplerLayoutBinding2 };

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = bindings.size();
			layoutInfo.pBindings = &bindings[0];

			auto res = vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_MaterialDescriptorSetLayout);
			assert(res == VK_SUCCESS);
		}
		
		{
			VkDescriptorSetLayoutBinding uboLayoutBinding = {};
			uboLayoutBinding.binding = 0;
			uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboLayoutBinding.descriptorCount = 1;
			uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

			array<VkDescriptorSetLayoutBinding, 1> bindings = { uboLayoutBinding };

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = bindings.size();
			layoutInfo.pBindings = &bindings[0];

			auto res = vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_SceneDescriptorSetLayout);

			assert(res == VK_SUCCESS);
		}
	}

	void createSyncPrimitives()
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			auto res = vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]);
			assert(res == VK_SUCCESS);

			res = vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]);
			assert(res == VK_SUCCESS);

			res = vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]);
			assert(res == VK_SUCCESS);
		}
		
	}

	vector<VkCommandBuffer> allocateCommandBuffers(uint32_t count)
	{
		vector<VkCommandBuffer> output(count);

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)output.size();

		auto res = vkAllocateCommandBuffers(m_Device, &allocInfo, &output[0]);
		assert(res == VK_SUCCESS);

		return output;
	}

	void recordCommandBuffer(VkCommandBuffer& commandBuffer, VkFramebuffer frameBuffer)
	{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			beginInfo.pInheritanceInfo = nullptr; // Optional

			auto res = vkBeginCommandBuffer(commandBuffer, &beginInfo);
			assert(res == VK_SUCCESS);

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_RenderPass;
			renderPassInfo.framebuffer = frameBuffer;
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = m_SwapChainExtent;

			array<VkClearValue, 2> clearValues = {};
			clearValues[0].color = { 72.0f / 255.0f, 201.0f / 255.0f, 176.0f / 255.0f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };
			
			renderPassInfo.clearValueCount = clearValues.size();
			renderPassInfo.pClearValues = &clearValues[0];

			vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

			//static glm::vec3 moveDir(1.0f, 3.0f, 2.0f);
			static glm::vec3 moveDir(0.0f, 0.0f, 0.0f);

			m_SceneDescription->m_Data.lightPos = glm::vec3(0.0f, 0.0f, 100.0f);
			//m_SceneDescription->m_Data.lightPos += moveDir;

			if ( abs(m_SceneDescription->m_Data.lightPos.x) > 500.0f) 
				moveDir.x = -moveDir.x;
			if (abs(m_SceneDescription->m_Data.lightPos.z) > 500.0f)  
				moveDir.z = -moveDir.z;

			if (m_SceneDescription->m_Data.lightPos.y > 1000.0f || m_SceneDescription->m_Data.lightPos.y < 100.0f)  
				moveDir.y = -moveDir.y;


			m_SceneDescription->m_Data.lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
			m_SceneDescription->m_Data.lightCount = 1;
			m_SceneDescription->m_Data.cameraPos = m_Camera.getMatrix()[3];

			m_SceneDescription->m_UniformBuffer.updateBuffer(&m_SceneDescription->m_Data, 1);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 1, 1, &m_SceneDescription->m_DescriptorSet.m_DescriptorSet, 0, nullptr);

			m_Scene.enumerate([&](SceneObject* sceneObj)
				{
					array<glm::mat4, 3> matrices = { m_ProjectionMatrix, m_Camera.getInvMatrix(), sceneObj->getMatrix() };
					vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(matrices), &matrices[0]);

					VisualComponent const* visualComp = sceneObj->findComponent<VisualComponent>();
					if (visualComp) visualComp->draw(commandBuffer, m_PipelineLayout);
				});

			vkCmdEndRenderPass(commandBuffer);

			res = vkEndCommandBuffer(commandBuffer);
			assert(res == VK_SUCCESS);
	}

	void createCommandBuffers()
	{
		m_CommandBuffers = allocateCommandBuffers(m_SwapChainFramebuffers.size());

		for (size_t i = 0; i < m_CommandBuffers.size(); i++) 
			recordCommandBuffer(m_CommandBuffers[i], m_SwapChainFramebuffers[i]);
	}

	void createCommandPool()
	{
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		auto res = vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool);
		assert(res == VK_SUCCESS);
	}

	void createFramebuffers()
	{
		m_SwapChainFramebuffers.reserve(m_SwapChainImageViews.size());

		for (auto imageView : m_SwapChainImageViews) 
		{
			array<VkImageView, 2> attachments = { imageView, m_DepthImageView };

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;
			framebufferInfo.attachmentCount = attachments.size();
			framebufferInfo.pAttachments = &attachments[0];
			framebufferInfo.width = m_SwapChainExtent.width;
			framebufferInfo.height = m_SwapChainExtent.height;
			framebufferInfo.layers = 1;

			VkFramebuffer frameBuffer;
			auto res = vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &frameBuffer);
			m_SwapChainFramebuffers.push_back(frameBuffer);

			assert(res == VK_SUCCESS);
		}
	}
	string readFile(const std::string& filename) const
	{
		ifstream file(filename, ifstream::binary);

		assert(file);

		file.seekg(0, file.end);
		size_t size = file.tellg();
		string buffer;
		buffer.resize(size);

		file.seekg(0, file.beg);
		file.read(&buffer[0], size);
		file.close();

		return buffer;
	}

	VkShaderModule createShaderModule(string const& binCode) 
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = binCode.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(binCode.data());

		VkShaderModule shaderModule;
		auto res = vkCreateShaderModule(m_Device, &createInfo, nullptr, &shaderModule);
		assert(res == VK_SUCCESS);
		return shaderModule;
	}

	void createRenderPass()
	{
		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = m_SwapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = attachments.size();
		renderPassInfo.pAttachments = &attachments[0];
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		auto res = vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass);
		assert(res == VK_SUCCESS);

	}
	void createGraphicsPipeline()
	{
		auto vertShaderCode = readFile("./../Shaders/vert.spv");
		auto fragShaderCode = readFile("./../Shaders/frag.spv");

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
		vertexInputInfo.pVertexAttributeDescriptions = &attributeDescriptions[0];

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_SwapChainExtent.width;
		viewport.height = (float)m_SwapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_SwapChainExtent;

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

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 2;

		array<VkDescriptorSetLayout, 2> setLayouts = { m_MaterialDescriptorSetLayout, m_SceneDescriptorSetLayout };
		pipelineLayoutInfo.pSetLayouts = &setLayouts[0];

		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.size = sizeof(glm::mat4) * 3;
		pushConstantRange.offset = 0;

		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		auto res = vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout);
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


		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; // Optional
		pipelineInfo.layout =m_PipelineLayout;
		pipelineInfo.renderPass = m_RenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		res = vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline);
		assert(res == VK_SUCCESS);

		vkDestroyShaderModule(m_Device, fragShaderModule, nullptr);
		vkDestroyShaderModule(m_Device, vertShaderModule, nullptr);
	}

	void createImageViews()
	{
		m_SwapChainImageViews.resize(m_SwapChainImages.size());
		int i = 0;
		for (auto image : m_SwapChainImages)
		{
			VulkanHelpers::createImageView(m_SwapChainImageViews[i++], m_Device, image, m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}

	}

	unsigned int getQueueFamilyIndex(int flags) const
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);

		vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

		auto it = find_if(begin(queueFamilies), end(queueFamilies), [&flags](VkQueueFamilyProperties const& properties)
			{
				return properties.queueFlags & flags;
			});

		assert(it != end(queueFamilies));

		return distance(begin(queueFamilies), it);
	}

	void createSurface(GLFWwindow &window)
	{
		VkResult res = glfwCreateWindowSurface(m_Instance, &window, nullptr, &m_Surface);
		assert(res == VK_SUCCESS);
	}

	vector<const char*> getRequiredExtensions()  const
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifndef NDEBUG //debug mode only
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

		return extensions;
	}

	void createInstance()
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Vulcan";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Vulcan Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

#ifndef NDEBUG //debug mode only
		vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
		createInfo.ppEnabledLayerNames = &validationLayers[0];
		createInfo.enabledLayerCount = validationLayers.size();
#endif

		vector<const char*>  extensions =getRequiredExtensions();
		createInfo.ppEnabledExtensionNames = &extensions[0];
		createInfo.enabledExtensionCount = extensions.size();

		VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
		assert(result == VK_SUCCESS);
	}

	void selectPhysicalDevice(vector<char const*> const & extensions)
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
		assert(deviceCount != 0);

		vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

		for (auto physicalDevice : devices)
		{
			if (checkPhysicalDeviceExtensions(physicalDevice, extensions))
			{
				m_PhysicalDevice = physicalDevice;
				return;
			}
		}
		
		assert(!"No available physical device with requested extensions");
	}

	bool checkPhysicalDeviceExtensions(VkPhysicalDevice physicalDevice, vector<char const*> const &requiredExtensions)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

		vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

		for (auto requiredExt : requiredExtensions)
		{
			if (end(availableExtensions) == find_if(begin(availableExtensions), end(availableExtensions), [requiredExt](VkExtensionProperties const &val)
				{
					return strcmp(val.extensionName, requiredExt) == 0;
				})) return false;
		}
		
		return true;
	}

	void createDevice(vector<const char*> const& deviceExtensions)
	{
		unsigned int const queueFamilyIndex = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);

		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueCount = 1;

		float const queuePriority = 1.0f;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		VkPhysicalDeviceFeatures physicalDeviceFeatures;
		vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &physicalDeviceFeatures);

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
		deviceCreateInfo.ppEnabledExtensionNames = &deviceExtensions[0];
		deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();

		VkResult result = vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device);
		assert(result == VK_SUCCESS);

		vkGetDeviceQueue(m_Device, queueFamilyIndex, 0, &m_GraphicQueue);
		m_PresentationQueue = m_GraphicQueue;

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, queueFamilyIndex, m_Surface, &presentSupport);
		assert(presentSupport == true);
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(vector<VkSurfaceFormatKHR> const& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(vector<VkPresentModeKHR> const& availablePresentModes)
	{
		if (end(availablePresentModes) != find(begin(availablePresentModes), end(availablePresentModes), VK_PRESENT_MODE_MAILBOX_KHR))
			return VK_PRESENT_MODE_MAILBOX_KHR;
		else
			return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.currentExtent;
		}
		else
		{
			VkExtent2D actualExtent = { 800, 600 }; //TODO

			actualExtent.width = clamp(capabilities.minImageExtent.width, capabilities.maxImageExtent.width, actualExtent.width);
			actualExtent.height = clamp(capabilities.minImageExtent.height, capabilities.maxImageExtent.height, actualExtent.height);

			return actualExtent;
		}
	}

	void createSwapChain()
	{
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &capabilities);
		VkExtent2D extent = chooseSwapExtent(capabilities);
		m_SwapChainExtent = extent;

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr);
		assert(presentModeCount != 0);
		
		vector<VkPresentModeKHR> presentModes(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, &presentModes[0]);

		VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, nullptr);
		assert(formatCount != 0);

		vector<VkSurfaceFormatKHR> formats(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, &formats[0]);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats);
		m_SwapChainImageFormat = surfaceFormat.format;

		uint32_t imageCount = min(capabilities.minImageCount + 1, capabilities.maxImageCount);
		assert(imageCount != 0);

		assert(m_PresentationQueue == m_GraphicQueue);

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
		createInfo.preTransform = capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VkResult res = vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_SwapChain);
		assert(res == VK_SUCCESS);

		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, nullptr);
		m_SwapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, &m_SwapChainImages[0]);
	}

	VkInstance m_Instance = VK_NULL_HANDLE;
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device = VK_NULL_HANDLE;
	VkQueue m_GraphicQueue = VK_NULL_HANDLE;
	VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
	VkQueue m_PresentationQueue = VK_NULL_HANDLE;
	VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
	vector<VkImage> m_SwapChainImages;
	VkFormat m_SwapChainImageFormat;
	VkExtent2D m_SwapChainExtent;
	vector<VkImageView> m_SwapChainImageViews;
	VkPipelineLayout m_PipelineLayout;
	VkRenderPass m_RenderPass;

	VkDescriptorSetLayout m_MaterialDescriptorSetLayout;
	VkDescriptorSetLayout m_SceneDescriptorSetLayout;

	VkPipeline m_GraphicsPipeline;
	vector<VkFramebuffer> m_SwapChainFramebuffers;
	VkCommandPool m_CommandPool;
	vector<VkCommandBuffer> m_CommandBuffers;

	VkImage m_DepthImage;
	VkDeviceMemory m_DepthImageMemory;
	VkImageView m_DepthImageView;

	static const size_t MAX_FRAMES_IN_FLIGHT = 2;
	array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_ImageAvailableSemaphores;
	array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_RenderFinishedSemaphores;
	array<VkFence, MAX_FRAMES_IN_FLIGHT> m_InFlightFences;
	size_t m_CurrentFrame = 0;
	
	glm::mat4x4 m_ProjectionMatrix;

	
	unique_ptr<SceneObjectFactory> m_SceneObjectFactory;
	SceneObjectManager m_Scene;

	unique_ptr < SceneDescription> m_SceneDescription;
	public:
	Camera m_Camera;
};


array<bool, 128> keyDown;
bool mouseButtonLeftDown = false;
double mouseX, mouseY;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS) keyDown[key] = true;
	else if (action == GLFW_RELEASE) keyDown[key] = false;
}


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{

		mouseButtonLeftDown = true;
		glfwSetCursorPos(window, 800.0f / 2, 600.0f / 2);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		mouseX = 400.0f;
		mouseY = 300.0f;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		mouseButtonLeftDown = false;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		glfwSetCursorPos(window, 800.0f / 2, 600.0f / 2);
	}
}

Window window(800, 600, "Vulkan");
VulcanInstance vulcanInstance(window.getWindow());

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (mouseButtonLeftDown)
	{
		float diffX = xpos - mouseX;
		float diffY = ypos - mouseY;

		mouseX = xpos;
		mouseY = ypos;

		vulcanInstance.m_Camera.rotate( diffY * 0.01f, -diffX * 0.01f);
	}
}

int main() 
{

	glfwSetKeyCallback(&window.getWindow(), key_callback);
	glfwSetMouseButtonCallback(&window.getWindow(), mouse_button_callback);
	glfwSetCursorPosCallback(&window.getWindow(), cursor_position_callback);

	while (!window.shouldClose())
	{
		window.pollEvents();
		vulcanInstance.drawFrame();
		const float speed = 10.0f;
		if (keyDown['W']) vulcanInstance.m_Camera.move(speed, 0.0f, 0.0f);
		if (keyDown['S']) vulcanInstance.m_Camera.move(-speed, 0.0f, 0.0f);
		if (keyDown['A']) vulcanInstance.m_Camera.move(0.0f, -speed, 0.0f);
		if (keyDown['D']) vulcanInstance.m_Camera.move(0.0f, speed, 0.0f);
	}


	return 0;
};