#pragma once

#include "SceneObject.h"
#include "ModelManager.h"
#include <vector>

using namespace std;

class VisualComponent : public IComponent
{
public:

	ModelDataSharedPtr m_ModelData;

	virtual ComponentType GetComponentType() override
	{
		return GetStaticComponentType();
	}

	static ComponentType GetStaticComponentType()
	{
		return ComponentType::VISUAL_COMPONENT;
	}

	VisualComponent(ModelDataSharedPtr const& model)
	{
		m_ModelData = model;
	}

	void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const
	{
		for (size_t i = 0; i < m_ModelData->meshes.size(); ++i)
		{
			auto descriptorSet = m_ModelData->materials[i]->m_ShaderParams.getDescriptorSet();
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_ModelData->meshes[i].vertexBuffer.buffer, offsets);

			vkCmdBindIndexBuffer(commandBuffer, m_ModelData->meshes[i].indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_ModelData->meshes[i].indexBuffer.count), 1, 0, 0, 0);
		}
	}


};