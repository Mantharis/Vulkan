#pragma once

#include "ParticleRenderer.h"
#include "ParticleComponent.h"


	ParticleRenderer::ParticleRenderer(VkPhysicalDevice physicalDevice, VkDevice device, VkRenderPass renderPass, int computeQueueIndex) : m_ParticleRendererData(device)
	{
		m_Device = device;
		m_RenderPass = renderPass;
		createComputePipeline(physicalDevice, device, computeQueueIndex);
		createRenderPipeline(physicalDevice, device, renderPass);
	}

	void ParticleRenderer::recordComputeCommand(vector< ParticleComponent*> const& particleComponents)
	{
		VkCommandBufferBeginInfo cmdBufferBeginInfo{};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		auto res = vkBeginCommandBuffer(m_ComputeCommandBuffer, &cmdBufferBeginInfo);
		assert(VK_SUCCESS == res);


		vkCmdBindPipeline(m_ComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);

		for (ParticleComponent const* particleComp : particleComponents)
		{
			for (auto& group : particleComp->m_ParticleGroups)
			{
				auto const& descriptorSet = group.m_DescriptorSetCompute.getDescriptorSet();
				vkCmdBindDescriptorSets(m_ComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayoutCompute, 0, 1, &descriptorSet, 0, 0);
				vkCmdDispatch(m_ComputeCommandBuffer, group.m_Particles.count / ParticleRendererData::PARTICLE_CNT_PER_GROUP, 1, 1);
			}
		}

		vkEndCommandBuffer(m_ComputeCommandBuffer);
	}

	void ParticleRenderer::submitComputeCommand()
	{
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_ComputeCommandBuffer;

		vkQueueSubmit(m_Queue, 1, &submitInfo, m_Fence);
		vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_Device, 1, &m_Fence);
	}


	void ParticleRenderer::createComputePipeline(VkPhysicalDevice physicalDevice, VkDevice device, int computeQueueIndex)
	{
		m_ParticleRendererData.m_ParticleSetLayoutCompute = make_shared< DescriptorSetLayout>(device);
		m_ParticleRendererData.m_ParticleSetLayoutCompute->addDescriptor("particles", 0, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		m_ParticleRendererData.m_ParticleSetLayoutCompute->createDescriptorSetLayout();

		// Create pipeline		

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 1;

		auto layout = m_ParticleRendererData.m_ParticleSetLayoutCompute->getLayout();
		pipelineLayoutCreateInfo.pSetLayouts = &layout;

		auto res = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayoutCompute);

		VkPipelineShaderStageCreateInfo shaderStage = {};
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStage.pName = "main"; // todo : make param
		VulkanHelpers::createShaderModuleFromFile("./../Shaders/particleCompute.spv", device, shaderStage.module);

		VkComputePipelineCreateInfo computePipelineCreateInfo{};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.layout = m_PipelineLayoutCompute;;
		computePipelineCreateInfo.flags = 0;
		computePipelineCreateInfo.stage = shaderStage;

		VkCommandPool commandPool;

		// Separate command pool as queue family for compute may be different than graphics
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = computeQueueIndex; // vulcanInstance.getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		res = vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &commandPool);
		assert(VK_SUCCESS == res);

		vkGetDeviceQueue(device, computeQueueIndex /*vulcanInstance.getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT)*/, 0, &m_Queue);

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


	void ParticleRenderer::createRenderPipeline(VkPhysicalDevice physicalDevice, VkDevice device, VkRenderPass renderPass)
	{
		m_ParticleRendererData.m_ParticleSetLayout = make_shared< DescriptorSetLayout>(device);
		m_ParticleRendererData.m_ParticleSetLayout->addDescriptor("particles", 0, VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		m_ParticleRendererData.m_ParticleSetLayout->addDescriptor("texture", 1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_ParticleRendererData.m_ParticleSetLayout->createDescriptorSetLayout();

		//GRAPHIC PIPELINE
		ShaderSet shaderData;

		shaderData.vertexShaderPath = string("./../Shaders/particleShaderVert.spv");
		shaderData.fragmentShaderPath = string("./../Shaders/particleShaderFrag.spv");
		shaderData.pushConstant.resize(1);
		shaderData.pushConstant[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		shaderData.pushConstant[0].offset = 0;
		shaderData.pushConstant[0].size = sizeof(glm::mat4);
		shaderData.descriptorSetLayout.push_back(m_ParticleRendererData.m_ParticleSetLayout->getLayout());

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;


		//particleRenderPipelineLayout, particleRenderPipeline
		VulkanHelpers::createGraphicsPipeline(device, renderPass, 1, { 1024,1024 }, shaderData, false, inputAssembly, m_ParticleRenderPipelineLayout, m_ParticleRenderPipeline);
	}
