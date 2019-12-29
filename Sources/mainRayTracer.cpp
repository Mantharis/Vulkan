
#include "mainRayTracer.h"

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

#include "Window.h"
#include "VulkanInstance.h"
#include "DeferredRenderer.h"
#include "ShadowRenderer.h"
#include "ParticleRenderer.h"
#include "ParticleComponent.h"

#include "RayTracer.h"
#include "mainCommon.h"

 using namespace std;



void runRayTracing()
{
	camera = new Camera();
	camera->setPos(glm::vec3(0.0f, 0.0f, 200.0f));
	camera->setDir(glm::vec3(0.0f, 0.0f, -1.0f));

	Window window(resX, resY, "Vulkan");
	VulcanInstance vulcanInstance(window.getWindow(), resX, resY);
	RayTracer rayTracer(vulcanInstance.m_PhysicalDevice, vulcanInstance.m_Device, vulcanInstance.getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT), vulcanInstance.m_CommandPool, vulcanInstance.m_GraphicQueue, vulcanInstance.m_RenderPass, 256, 256);
	SceneObjectFactory sceneObjectFactory(vulcanInstance.m_PhysicalDevice, vulcanInstance.m_Device, vulcanInstance.m_CommandPool, vulcanInstance.m_GraphicQueue);


	unique_ptr<SceneObject> obj = sceneObjectFactory.createSceneObjectFromFile("./../Models/test7/Model.obj");
	glm::mat4& mat = obj->getMatrix();
	mat[0][0] = mat[1][1] = mat[2][2] = 100.0f;
	mat[3][3] = 1.0f;
	mat[3][0] = 0.0f;

	VisualComponent* visualComp = obj->findComponent<VisualComponent>();

	vector<Triangle> triangles;

	for (size_t i = 0; i < visualComp->m_ModelData->meshes.size(); ++i)
	{
		unsigned int* indices = static_cast<unsigned int*>(visualComp->m_ModelData->meshes[i].indexBuffer.mapMemory());
		Vertex* vertices = static_cast<Vertex*>(visualComp->m_ModelData->meshes[i].vertexBuffer.mapMemory());

		for (size_t idx = 0; idx < visualComp->m_ModelData->meshes[i].indexBuffer.count; idx += 3)
		{
			Triangle triangle;

			triangle.v0 = mat * glm::vec4(vertices[indices[idx]].pos, 1.0f);
			triangle.v1 = mat * glm::vec4(vertices[indices[idx + 1]].pos, 1.0f);
			triangle.v2 = mat * glm::vec4(vertices[indices[idx + 2]].pos, 1.0f);

			triangle.t0 = vertices[indices[idx]].texCoord;
			triangle.t1 = vertices[indices[idx + 1]].texCoord;
			triangle.t2 = vertices[indices[idx + 2]].texCoord;

			triangles.push_back(triangle);
		}

		visualComp->m_ModelData->meshes[i].indexBuffer.unmapMemory();
		visualComp->m_ModelData->meshes[i].vertexBuffer.unmapMemory();
	}


	rayTracer.setTriangles(triangles);
	rayTracer.setTexture(visualComp->m_ModelData->materials[0]->m_DiffuseTexture->imageView);

		


	window.setKeyCallback(key_callback);
	window.setMouseMoveCallback(cursor_position_callback);
	window.setMouseButtonCallback(mouse_button_callback);


	volatile static bool restart = false;
	while (!window.shouldClose())
	{
		window.pollEvents();

		vulcanInstance.drawFrame([ &rayTracer](VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer, VkImage image)
			{
				rayTracer.setView(camera->getMatrix());

				rayTracer.recordComputeCommand();
				rayTracer.submitComputeCommand();

				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				beginInfo.pInheritanceInfo = nullptr; // Optional

				auto res = vkBeginCommandBuffer(commandBuffer, &beginInfo);
				assert(res == VK_SUCCESS);

				VkRenderPassBeginInfo renderPassInfo = {};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassInfo.renderPass = rayTracer.m_RenderPass;
				renderPassInfo.framebuffer = frameBuffer;
				renderPassInfo.renderArea.offset = { 0, 0 };
				renderPassInfo.renderArea.extent = { 1024,1024 };

				array<VkClearValue, 2> clearValues = {};
				clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
				clearValues[1].depthStencil = { 1.0f, 0 };

				renderPassInfo.clearValueCount = 2;
				renderPassInfo.pClearValues = &clearValues[0];

				vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				VkClearAttachment clearAtt;
				clearAtt.colorAttachment = 0;
				clearAtt.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				clearAtt.clearValue = clearValues[0];

				VkClearRect rect;
				rect.baseArrayLayer = 0;
				rect.layerCount = 1;
				rect.rect.offset = { 0,0 };
				rect.rect.extent = { 1024,1024 };
				vkCmdClearAttachments(commandBuffer, 1, &clearAtt, 1, &rect);

				VkDescriptorSet  descSet = rayTracer.m_RenderDescriptor->getDescriptorSet();
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rayTracer.m_RenderPipelineLayout, 0, 1, &descSet, 0, NULL);
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rayTracer.m_RenderPipeline);

				//draw quad
				vkCmdDraw(commandBuffer, 6, 1, 0, 0);

				vkCmdEndRenderPass(commandBuffer);
				res = vkEndCommandBuffer(commandBuffer);
				assert(res == VK_SUCCESS);

			});


		const float speed = 10.0f;
		if (keyDown['E']) camera->move(0.0f, 0.0f, speed);
		if (keyDown['Q']) camera->move(0.0f, 0.0f, -speed);
		if (keyDown['W']) camera->move(speed, 0.0f, 0.0f);
		if (keyDown['S']) camera->move(-speed, 0.0f, 0.0f);
		if (keyDown['A']) camera->move(0.0f, -speed, 0.0f);
		if (keyDown['D']) camera->move(0.0f, speed, 0.0f);

	}

	delete camera;
	camera = nullptr;

}

