#pragma once

#include "ParticleComponent.h"
#include "VertexFormat.h"
#include "ParticleRenderer.h"

	vector<Particle2> ParticleComponent::loadParticles(size_t i, ModelDataSharedPtr& modelDataSharedPtr, glm::vec3 const& centerOfGravityOffset)
	{
		vector<Particle2> particles;

		vector<Vertex> vertices(modelDataSharedPtr->meshes[i].vertexBuffer.count);
		vector<uint32_t> indices(modelDataSharedPtr->meshes[i].indexBuffer.count);

		modelDataSharedPtr->meshes[i].vertexBuffer.copyBuffer(&vertices[0]);
		modelDataSharedPtr->meshes[i].indexBuffer.copyBuffer(&indices[0]);

		glm::vec3 centerOfGravity = glm::vec3(0.0f);
		float totalMass = 0.0f;

		for (uint32_t ind = 0; ind < indices.size(); ind += 3)
		{
			Particle2 particle;
			particle.v0 = glm::vec4(vertices[indices[ind]].pos, 1.0f);
			particle.v1 = glm::vec4(vertices[indices[ind + 1]].pos, 1.0f);
			particle.v2 = glm::vec4(vertices[indices[ind + 2]].pos, 1.0f);

			particle.t0 = vertices[indices[ind]].texCoord;
			particle.t1 = vertices[indices[ind + 1]].texCoord;
			particle.t2 = vertices[indices[ind + 2]].texCoord;

			particle.mass = glm::length(glm::cross(particle.v1 - particle.v0, particle.v2 - particle.v1)) * 0.5f;
			particles.push_back(particle);

			centerOfGravity += particle.mass * (particle.v0 + particle.v1 + particle.v2) / 3.0f;
			totalMass += particle.mass;
		};

		centerOfGravity /= totalMass;

		centerOfGravity += centerOfGravityOffset;
		for (auto& particle : particles)
		{
			glm::vec3 particlePos = (particle.v0 + particle.v1 + particle.v2) / 3.0f;

			float dis = glm::length(particlePos - centerOfGravity);
			float energyAttenuation = 1.0f / (dis * dis);
			float forceMagnitude = 0.0003f; // 300000.0f;

			particle.velocity = normalize(particlePos - centerOfGravity) * energyAttenuation * forceMagnitude / particle.mass;
			particle.velocity = particle.velocity; // glm::vec3(0.0, 0.0f, 0.0f);
		}

		while (particles.size() % ParticleRendererData::PARTICLE_CNT_PER_GROUP != 0)
		{
			particles.push_back(Particle2());
		}

		return particles;
	}

	
	ParticleComponent::ParticleComponent(VkPhysicalDevice physicalDevice, VkDevice device, ModelDataSharedPtr& modelDataSharedPtr, glm::vec3 const& centerOfGravityOffset, ParticleRendererData const &particleRendererData)
	{
		for (size_t i = 0; i < modelDataSharedPtr->meshes.size(); ++i)
		{
			auto particles = loadParticles(i, modelDataSharedPtr, centerOfGravityOffset);

			Buffer particleBuffer(physicalDevice, device, &particles[0], particles.size(), (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT));

			DescriptorSet particleSetCompute(particleRendererData.m_ParticleSetLayoutCompute);
			particleSetCompute.createDescriptorSet();

			particleSetCompute.setStorage("particles", particleBuffer);
			

			DescriptorSet particleSet(particleRendererData.m_ParticleSetLayout);
			particleSet.createDescriptorSet();

			particleSet.setSampler("texture", modelDataSharedPtr->materials[0]->m_DiffuseTexture->imageView, particleRendererData.m_sampler.m_Sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			particleSet.setStorage("particles", particleBuffer);
			

			m_ParticleGroups.emplace_back(move(particleSet), move(particleSetCompute), modelDataSharedPtr->materials[i]->m_DiffuseTexture, move(particleBuffer));
		}
	}

	void ParticleComponent::reset(ParticleComponent& particleComp, ModelDataSharedPtr& modelDataSharedPtr, glm::vec3 const& centerOfGravityOffset)
	{
		for (size_t i = 0; i < modelDataSharedPtr->meshes.size(); ++i)
		{
			auto particles = loadParticles(i, modelDataSharedPtr, centerOfGravityOffset);
			particleComp.m_ParticleGroups[i].m_Particles.updateBuffer(&particles[0]);
		}
	}
