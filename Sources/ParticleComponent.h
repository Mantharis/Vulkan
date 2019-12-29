
#include "SceneObject.h"
#include <glm/vec3.hpp>
#include "VulkanHelper.h"
#include "MaterialManager.h"
#include "ModelManager.h"
#include <memory>

using namespace std;

struct Particle2
{
	alignas(16) glm::vec3 v0;
	alignas(16) glm::vec3 v1;
	alignas(16) glm::vec3 v2;
	alignas(16) glm::vec3 velocity;

	alignas(8) glm::vec2 t0;
	alignas(8) glm::vec2 t1;
	alignas(8) glm::vec2 t2;

	alignas(4) float mass;
};


struct ParticleGroup
{
	DescriptorSet m_DescriptorSet;
	DescriptorSet m_DescriptorSetCompute;
	shared_ptr<const TextureData> m_Texture;
	Buffer m_Particles;

	ParticleGroup(DescriptorSet&& descriptorSet, DescriptorSet&& descriptorSetCompute, shared_ptr<const TextureData> texture, Buffer&& buffer) :
		m_DescriptorSet(move(descriptorSet)), m_DescriptorSetCompute(move(descriptorSetCompute)), m_Texture(move(texture)), m_Particles(move(buffer))
	{};
};

struct ParticleRendererData;

class ParticleComponent : public IComponent
{
public:

	ParticleComponent(VkPhysicalDevice physicalDevice, VkDevice device, ModelDataSharedPtr& modelDataSharedPtr, glm::vec3 const& centerOfGravityOffset, ParticleRendererData const& particleRendererData);
	void reset(ParticleComponent& particleComp, ModelDataSharedPtr& modelDataSharedPtr, glm::vec3 const& centerOfGravityOffset);


	ComponentType GetComponentType() override
	{
		return GetStaticComponentType();
	}

	static ComponentType GetStaticComponentType()
	{
		return ComponentType::PARTICLE_COMPONENT;
	}

	vector<ParticleGroup> m_ParticleGroups;

private:
	static vector<Particle2> loadParticles(size_t i, ModelDataSharedPtr& modelDataSharedPtr, glm::vec3 const& centerOfGravityOffset);
};