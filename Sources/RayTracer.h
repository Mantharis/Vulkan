#pragma once

#include "VulkanHelper.h"
#include "MaterialManager.h"
#include "Model.h"
#include <memory>

using namespace std;

struct Image
{
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;
	VkDevice device;

	Image(VkImage image, VkDeviceMemory memory, VkImageView view, VkDevice device):image(image), imageMemory(memory), imageView(view), device(device)
	{}

	~Image()
	{
		destroy();
	}

	Image& operator=(Image&& obj)
	{
		destroy();
		*this = obj;
		obj.device = VK_NULL_HANDLE;

		return *this;
	}

	Image(Image&& obj)
	{
		*this = move(obj);
	}

private:
	Image& operator=(Image const&) = default;
	Image(Image const&) = default;

	void destroy()
	{
		if (device != VK_NULL_HANDLE)
		{
			vkDestroyImageView(device, imageView, nullptr);
			vkDestroyImage(device, image, nullptr);
			vkFreeMemory(device, imageMemory, nullptr);
			device = VK_NULL_HANDLE;
		}
	}
};


struct Triangle
{
	alignas(16) glm::vec3 v0;
	alignas(16) glm::vec3 v1;
	alignas(16) glm::vec3 v2;

	alignas(8) glm::vec2 t0;
	alignas(8) glm::vec2 t1;
	alignas(8) glm::vec2 t2;

	unsigned int materialId;
};

struct Sphere
{
	alignas(16) glm::vec3 pos;
	float radius;
	unsigned int materialId;
};

struct Plane
{
	alignas(16) glm::vec3 normal;
	float paramD;

	static Plane ConstructPlaneFromTriangle(glm::vec3 const& p1, glm::vec3 const& p2, glm::vec3 const& p3)
	{
		//dot(n,p)+d=0
		Plane plane;

		plane.normal = glm::normalize(glm::cross(p2 - p1, p3 - p2));
		plane.paramD = -glm::dot(plane.normal, p1);
		
		return plane;
	}
};

struct Material
{
	alignas(16) glm::vec4 color;
	float reflFactor;
	int texId;
};

struct RayTracerUBO
{
	alignas(4) unsigned int resX = 0;
	alignas(4) unsigned int resY = 0;
	alignas(4) unsigned int triangleCnt = 0;
	alignas(4) unsigned int sphereCnt = 0;
};

class RayTracer
{
public:
	RayTracer(VkPhysicalDevice physicalDevice, VkDevice device, int computeQueueIndex, VkCommandPool commandPool, VkQueue queue, VkRenderPass renderpass, size_t resX, size_t resY);

	void setResolution(size_t x, size_t y);
	void setTriangles(vector<Triangle> const &triangles);
	void setSpheres(vector<Sphere> const& spheres);
	void setMaterials(vector<Material> const& materials);


	void setView(glm::mat4 const& matrix);
	void setTextures(vector<VkImageView> const& imageViews);

	void recordComputeCommand();
	void submitComputeCommand();

//private:

	static const size_t RAYS_PER_GROUP = 8;

	
	VkPipelineLayout m_PipelineLayoutCompute;
	VkPipeline m_ComputePipeline;
	VkCommandBuffer m_ComputeCommandBuffer;

	DescriptorSet m_ComputeDescriptorSet;

	unique_ptr<Buffer> m_TriangleBuffer;
	unique_ptr<Buffer>  m_SphereBuffer;
	unique_ptr<Buffer> m_MaterialBuffer;

	VkFence m_Fence;
	VkQueue m_Queue;
	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice;

	Sampler m_Sampler;
	unique_ptr< Image> m_DstImage;

	VkRenderPass m_RenderPass;
	VkPipeline m_RenderPipeline;
	VkPipelineLayout m_RenderPipelineLayout;
	shared_ptr<DescriptorSet>  m_RenderDescriptor;

	unique_ptr<Buffer> m_UniformBuffer;
	RayTracerUBO* m_UniformBufferMappingPtr;

	glm::mat4 m_ViewMatrix;

	void createRenderPass(VkPhysicalDevice physicalDevice, VkDevice device);
	void createComputePipeline(VkPhysicalDevice physicalDevice, VkDevice device, int computeQueueIndex);
}; 