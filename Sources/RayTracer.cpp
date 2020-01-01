#pragma once

#include "RayTracer.h"
#include "ParticleComponent.h"


	RayTracer::RayTracer(VkPhysicalDevice physicalDevice, VkDevice device, int computeQueueIndex, VkCommandPool commandPool, VkQueue queue, VkRenderPass renderPass, size_t resX, size_t resY):m_ComputeDescriptorSet(make_shared<DescriptorSetLayout>(device)), m_Sampler(device)
	{
		m_Device = device;
		m_PhysicalDevice = physicalDevice;
		m_RenderPass = renderPass;
		createComputePipeline(physicalDevice, device, computeQueueIndex);

		VkImage image;
		VkDeviceMemory imageMemory;
		VkImageView imageView;

		

		VulkanHelpers::createImage(physicalDevice, device, resX, resY, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);

		VulkanHelpers::transitionImageLayout(device, commandPool, queue, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		VulkanHelpers::createImageView(imageView, device, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

		m_DstImage = make_unique< Image>(image, imageMemory, imageView, m_Device);

		m_ComputeDescriptorSet.setImageStorage("dstImage", imageView, m_Sampler.m_Sampler, VK_IMAGE_LAYOUT_GENERAL);

		RayTracerUBO settings;
		m_UniformBuffer = make_unique<Buffer>(m_PhysicalDevice, m_Device, &settings, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		m_ComputeDescriptorSet.setBuffer("settings", *m_UniformBuffer);
		m_UniformBufferMappingPtr = static_cast<RayTracerUBO*>(m_UniformBuffer->mapMemory());
		m_UniformBufferMappingPtr->resX = resX;
		m_UniformBufferMappingPtr->resY = resY;

		createRenderPass(m_PhysicalDevice, m_Device);
	}

	void RayTracer::setTexture(VkImageView imageView)
	{
		m_ComputeDescriptorSet.setSampler("texture", imageView, m_Sampler.m_Sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void RayTracer::setView(glm::mat4 const& matrix)
	{
		//m_ViewMatrix = glm::mat4(1.0f);
		m_ViewMatrix = matrix;
	}

	void RayTracer::recordComputeCommand()
	{
		VkCommandBufferBeginInfo cmdBufferBeginInfo{};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		auto res = vkBeginCommandBuffer(m_ComputeCommandBuffer, &cmdBufferBeginInfo);
		assert(VK_SUCCESS == res);

		vkCmdBindPipeline(m_ComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);

		vkCmdPushConstants(m_ComputeCommandBuffer, m_PipelineLayoutCompute, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(m_ViewMatrix), &m_ViewMatrix);
			
		auto const& descriptorSet = m_ComputeDescriptorSet.getDescriptorSet();
		vkCmdBindDescriptorSets(m_ComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayoutCompute, 0, 1, &descriptorSet, 0, 0);
		vkCmdDispatch(m_ComputeCommandBuffer, (m_UniformBufferMappingPtr->resX * m_UniformBufferMappingPtr->resY) / RAYS_PER_GROUP, 1, 1);
			
		vkEndCommandBuffer(m_ComputeCommandBuffer);
	}

	void RayTracer::setTriangles(vector<Triangle>  const & triangles)
	{
		m_TriangleBuffer = make_unique<Buffer>(m_PhysicalDevice, m_Device, &triangles[0], triangles.size(), (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT));
		m_ComputeDescriptorSet.setStorage("triangles", *m_TriangleBuffer);

		m_UniformBufferMappingPtr->triangleCnt = triangles.size();

		/*
		RayTracerUBO tmpBuffer;
		m_UniformBuffer->copyBuffer(&tmpBuffer);

		tmpBuffer.polygonCnt = triangles.size();

		m_UniformBuffer->updateBuffer(&tmpBuffer);
		*/
	}

	void RayTracer::setSpheres(vector<Sphere> const& spheres)
	{
		m_SphereBuffer = make_unique<Buffer>(m_PhysicalDevice, m_Device, &spheres[0], spheres.size(), (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT));
		m_ComputeDescriptorSet.setStorage("spheres", *m_SphereBuffer);

		m_UniformBufferMappingPtr->sphereCnt = spheres.size();
	}


	void RayTracer::setMaterials(vector<Material> const& materials)
	{
		m_MaterialBuffer = make_unique<Buffer>(m_PhysicalDevice, m_Device, &materials[0], materials.size(), (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT));
		m_ComputeDescriptorSet.setStorage("materials", *m_MaterialBuffer);
	}

	void RayTracer::submitComputeCommand()
	{
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_ComputeCommandBuffer;

		//viewMatrix

		vkQueueSubmit(m_Queue, 1, &submitInfo, m_Fence);
		vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_Device, 1, &m_Fence);
	}


	void RayTracer::createComputePipeline(VkPhysicalDevice physicalDevice, VkDevice device, int computeQueueIndex)
	{
		m_ComputeDescriptorSet.getDescriptorSetlayout()->addDescriptor("dstImage", 0, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		m_ComputeDescriptorSet.getDescriptorSetlayout()->addDescriptor("settings", 1, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		m_ComputeDescriptorSet.getDescriptorSetlayout()->addDescriptor("materials", 2, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		m_ComputeDescriptorSet.getDescriptorSetlayout()->addDescriptor("triangles", 3, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		m_ComputeDescriptorSet.getDescriptorSetlayout()->addDescriptor("spheres", 4, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		m_ComputeDescriptorSet.getDescriptorSetlayout()->addDescriptor("texture", 5, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		m_ComputeDescriptorSet.getDescriptorSetlayout()->createDescriptorSetLayout();
		m_ComputeDescriptorSet.createDescriptorSet();
	
		// Create pipeline		

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 1;

		auto layout = m_ComputeDescriptorSet.getDescriptorSetlayout()->getLayout();
		pipelineLayoutCreateInfo.pSetLayouts = &layout;

		
		VkPushConstantRange pushConstants[1];
		pushConstants[0].offset = 0;
		pushConstants[0].size = sizeof(glm::mat4);
		pushConstants[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstants[0];
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		

		auto res = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayoutCompute);

		VkPipelineShaderStageCreateInfo shaderStage = {};
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStage.pName = "main"; // todo : make param
		VulkanHelpers::createShaderModuleFromFile("./../Shaders/rayTracerCompute.spv", device, shaderStage.module);

		VkComputePipelineCreateInfo computePipelineCreateInfo{};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.layout = m_PipelineLayoutCompute;;
		computePipelineCreateInfo.flags = 0;
		computePipelineCreateInfo.stage = shaderStage;


		VkCommandPool commandPool;

		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = computeQueueIndex;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		res = vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &commandPool);
		assert(VK_SUCCESS == res);

		vkGetDeviceQueue(device, computeQueueIndex, 0, &m_Queue);

		// Fence for compute CB sync
		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		res = vkCreateFence(device, &fenceCreateInfo, nullptr, &m_Fence);
		assert(VK_SUCCESS == res);

		// Create a command buffer for compute operations
		VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = commandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = 1;

		res = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &m_ComputeCommandBuffer);
		assert(VK_SUCCESS == res);

		res = vkCreateComputePipelines(device, nullptr, 1, &computePipelineCreateInfo, nullptr, &m_ComputePipeline);
		assert(VK_SUCCESS == res);

		vkResetFences(device, 1, &m_Fence);
	}

	void RayTracer::createRenderPass(VkPhysicalDevice physicalDevice, VkDevice device)
	{
		//DESCRIPTOR SET
		shared_ptr<DescriptorSetLayout> descriptorSetLayout = make_shared< DescriptorSetLayout>(device);
		descriptorSetLayout->addDescriptor("image", 0, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorSetLayout->createDescriptorSetLayout();

		

		m_RenderDescriptor = make_shared< DescriptorSet>(descriptorSetLayout);
		m_RenderDescriptor->createDescriptorSet();

		m_RenderDescriptor->setSampler("image", m_DstImage->imageView,  m_Sampler.m_Sampler, VK_IMAGE_LAYOUT_GENERAL);
		
		//GRAPHIC PIPELINE
		ShaderSet shaderData;
		shaderData.vertexInputBindingDescription = {};
		shaderData.vertexShaderPath = string("./../Shaders/rayTracerVert.spv");
		shaderData.fragmentShaderPath = string("./../Shaders/rayTracerFrag.spv");

	
		shaderData.descriptorSetLayout.push_back(descriptorSetLayout->getLayout());

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VulkanHelpers::createGraphicsPipeline(m_Device, m_RenderPass, 1, { 1024, 1024 }, shaderData, true, inputAssembly, m_RenderPipelineLayout, m_RenderPipeline);
	}

	
	
