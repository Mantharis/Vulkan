#include "VulkanHelper.h"
#include <vector>
#include <functional>
#include <array>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using namespace std;


struct RenderContext
{
	VkRenderPass renderPass;
	VkFramebuffer framebuffer;
	VkImageView depthImageView;
	VkImageView colorImageView;
};

class VulcanInstance
{
public:
	VulcanInstance(GLFWwindow& window, size_t resX, size_t resY);
	~VulcanInstance();

	void drawFrame(function<void(VkCommandBuffer, VkFramebuffer, VkImage)> func);
	void createDepthResources();
	void createSyncPrimitives();
	vector<VkCommandBuffer> allocateCommandBuffers(uint32_t count);
	void createCommandBuffers();
	void createCommandPool();
	void createFramebuffers();
	string readFile(const std::string& filename) const;
	VkShaderModule createShaderModule(string const& binCode);
	void createRenderPass();
	void createImageViews();
	unsigned int getQueueFamilyIndex(int flags) const;
	void createSurface(GLFWwindow& window);
	vector<const char*> getRequiredExtensions()  const;
	void createInstance();
	bool checkPhysicalDeviceExtensions(VkPhysicalDevice physicalDevice, vector<char const*> const& requiredExtensions);
	void createDevice(vector<const char*> const& deviceExtensions);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(vector<VkSurfaceFormatKHR> const& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(vector<VkPresentModeKHR> const& availablePresentModes);
	void createSwapChain(VkExtent2D extent);
	void selectPhysicalDevice(vector<char const*> const& extensions);

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

	VkRenderPass m_RenderPass;

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
};