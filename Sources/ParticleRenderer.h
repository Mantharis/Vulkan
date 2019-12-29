
#include "VulkanHelper.h"
#include "MaterialManager.h"
#include "Model.h"
#include <memory>

using namespace std;

struct ParticleRendererData
{
	ParticleRendererData(Sampler&& sampler) :m_sampler(move(sampler)) {};

	Sampler m_sampler;
	shared_ptr<DescriptorSetLayout> m_ParticleSetLayout;
	shared_ptr<DescriptorSetLayout> m_ParticleSetLayoutCompute;
	static const int PARTICLE_CNT_PER_GROUP = 4;
};

class ParticleComponent;

class ParticleRenderer
{
public:
	VkPipelineLayout m_PipelineLayoutCompute;
	VkPipelineLayout m_ParticleRenderPipelineLayout;
	VkPipeline m_ParticleRenderPipeline;
	
	VkPipeline m_ComputePipeline;
	VkCommandBuffer m_ComputeCommandBuffer;
	VkFence m_Fence;
	VkQueue m_Queue;
	VkDevice m_Device;
	VkRenderPass m_RenderPass;

	ParticleRendererData m_ParticleRendererData;

	ParticleRenderer(VkPhysicalDevice physicalDevice, VkDevice device, VkRenderPass renderPass, int computeQueueIndex);

	void recordComputeCommand(vector< ParticleComponent*> const& particleComponents);
	void submitComputeCommand();

private:

	void createComputePipeline(VkPhysicalDevice physicalDevice, VkDevice device, int computeQueueIndex);
	void createRenderPipeline(VkPhysicalDevice physicalDevice, VkDevice device, VkRenderPass renderPass);
};