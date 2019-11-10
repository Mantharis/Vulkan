#include "VulkanInstance.h"
#include <fstream>

	VulcanInstance::VulcanInstance(GLFWwindow& window, size_t resX, size_t resY)
	{
		createInstance();
		createSurface(window);

		const vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		selectPhysicalDevice(deviceExtensions);

		createDevice(deviceExtensions);

		VkExtent2D extent = { static_cast<uint32_t>(resX), static_cast<uint32_t>(resY)};

		createSwapChain(extent);
		createImageViews();
		createRenderPass();
		createCommandPool();

		createDepthResources();
		createFramebuffers();
		createCommandBuffers();
		createSyncPrimitives();
	}

	VulcanInstance::~VulcanInstance()
	{
		vkDeviceWaitIdle(m_Device);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
		}



		//TODO 
		//m_Models[0].destroy();
		//m_Models.clear();

		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

		for (auto framebuffer : m_SwapChainFramebuffers) vkDestroyFramebuffer(m_Device, framebuffer, nullptr);

		//TODO
		//vkDestroyDescriptorSetLayout(m_Device, m_MaterialDescriptorSetLayout, nullptr);
		//vkDestroyDescriptorSetLayout(m_Device, m_SceneDescriptorSetLayout, nullptr);


		vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

		for (auto imageView : m_SwapChainImageViews) vkDestroyImageView(m_Device, imageView, nullptr);

		vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
		vkDestroyDevice(m_Device, nullptr);
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
	}

	
	void VulcanInstance::drawFrame(function<void(VkCommandBuffer, VkFramebuffer, VkImage)> func)
	{
		vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

		uint32_t imageIndex;
		auto res = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
		assert(res == VK_SUCCESS);

		func(m_CommandBuffers[imageIndex], m_SwapChainFramebuffers[imageIndex], m_SwapChainImages[imageIndex]);


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

	void VulcanInstance::createDepthResources()
	{
		VulkanHelpers::createImage(m_PhysicalDevice, m_Device, m_SwapChainExtent.width, m_SwapChainExtent.width, VK_FORMAT_D16_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);
		VulkanHelpers::createImageView(m_DepthImageView, m_Device, m_DepthImage, VK_FORMAT_D16_UNORM, VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	void VulcanInstance::createSyncPrimitives()
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

	vector<VkCommandBuffer> VulcanInstance::allocateCommandBuffers(uint32_t count)
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

	void VulcanInstance::createCommandBuffers()
	{
		m_CommandBuffers = allocateCommandBuffers(static_cast<uint32_t>(m_SwapChainFramebuffers.size()));

		/*
		for (size_t i = 0; i < m_CommandBuffers.size(); i++)
			recordCommandBuffer(m_CommandBuffers[i], m_SwapChainFramebuffers[i], func);
			*/
	}

	void VulcanInstance::createCommandPool()
	{
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		auto res = vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool);
		assert(res == VK_SUCCESS);
	}

	void VulcanInstance::createFramebuffers()
	{
		m_SwapChainFramebuffers.reserve(m_SwapChainImageViews.size());

		for (auto imageView : m_SwapChainImageViews)
		{
			array<VkImageView, 2> attachments = { imageView, m_DepthImageView };

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
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
	string VulcanInstance::readFile(const std::string& filename) const
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

	VkShaderModule VulcanInstance::createShaderModule(string const& binCode)
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

	void VulcanInstance::createRenderPass()
	{
		VulkanHelpers::createRenderPass(m_RenderPass, m_Device, m_SwapChainImageFormat, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_LOAD_OP_LOAD, 1, true, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}

	void VulcanInstance::createImageViews()
	{
		m_SwapChainImageViews.resize(m_SwapChainImages.size());
		int i = 0;
		for (auto image : m_SwapChainImages)
		{
			VulkanHelpers::createImageView(m_SwapChainImageViews[i++], m_Device, image, m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	unsigned int VulcanInstance::getQueueFamilyIndex(int flags) const
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

		return static_cast<uint32_t>(distance(begin(queueFamilies), it));
	}

	void VulcanInstance::createSurface(GLFWwindow& window)
	{
		VkResult res = glfwCreateWindowSurface(m_Instance, &window, nullptr, &m_Surface);
		assert(res == VK_SUCCESS);
	}

	vector<const char*> VulcanInstance::getRequiredExtensions()  const
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

	void VulcanInstance::createInstance()
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
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
#endif

		vector<const char*>  extensions = getRequiredExtensions();
		createInfo.ppEnabledExtensionNames = &extensions[0];
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());

		VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
		assert(result == VK_SUCCESS);
	}

	void VulcanInstance::selectPhysicalDevice(vector<char const*> const& extensions)
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

	bool VulcanInstance::checkPhysicalDeviceExtensions(VkPhysicalDevice physicalDevice, vector<char const*> const& requiredExtensions)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

		vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

		for (auto requiredExt : requiredExtensions)
		{
			if (end(availableExtensions) == find_if(begin(availableExtensions), end(availableExtensions), [requiredExt](VkExtensionProperties const& val)
				{
					return strcmp(val.extensionName, requiredExt) == 0;
				})) return false;
		}

		return true;
	}

	void VulcanInstance::createDevice(vector<const char*> const& deviceExtensions)
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
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());

		VkResult result = vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device);
		assert(result == VK_SUCCESS);

		vkGetDeviceQueue(m_Device, queueFamilyIndex, 0, &m_GraphicQueue);
		m_PresentationQueue = m_GraphicQueue;

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, queueFamilyIndex, m_Surface, &presentSupport);
		assert(presentSupport != 0);
	}

	VkSurfaceFormatKHR VulcanInstance::chooseSwapSurfaceFormat(vector<VkSurfaceFormatKHR> const& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR VulcanInstance::chooseSwapPresentMode(vector<VkPresentModeKHR> const& availablePresentModes)
	{
		if (end(availablePresentModes) != find(begin(availablePresentModes), end(availablePresentModes), VK_PRESENT_MODE_MAILBOX_KHR))
			return VK_PRESENT_MODE_MAILBOX_KHR;
		else
			return VK_PRESENT_MODE_FIFO_KHR;
	}

	void VulcanInstance::createSwapChain(VkExtent2D extent)
	{
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &capabilities);

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