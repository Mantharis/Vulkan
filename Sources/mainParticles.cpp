
#include "mainParticles.h"

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



 void runParticles()
 {
	 camera = new Camera();
	 camera->setPos(glm::vec3(0.0f, 0.0f, 200.0f));
	 camera->setDir(glm::vec3(0.0f, 0.0f, -1.0f));


	 Window window(resX, resY, "Vulkan");
	 VulcanInstance vulcanInstance(window.getWindow(), resX, resY);


	 ParticleRenderer particleRender(vulcanInstance.m_PhysicalDevice, vulcanInstance.m_Device, vulcanInstance.m_RenderPass, vulcanInstance.getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT));
	 SceneObjectFactory sceneObjectFactory(vulcanInstance.m_PhysicalDevice, vulcanInstance.m_Device, vulcanInstance.m_CommandPool, vulcanInstance.m_GraphicQueue);

	 static auto projectionMatrix = VulkanHelpers::preparePerspectiveProjectionMatrix((float)resX / resY, 60, 1.0f, 10000.0f);


	 SceneObjectManager sceneObjectManager;

	 ModelDataSharedPtr modelData;

	 {
		 unique_ptr<SceneObject> obj = sceneObjectFactory.createSceneObjectFromFile("./../Models/test7/Model.obj");
		 glm::mat4& mat = obj->getMatrix();
		 mat[0][0] = mat[1][1] = mat[2][2] = 100.0f;
		 mat[3][3] = 1.0f;
		 mat[3][0] = 0.0f;

		 VisualComponent* visualComp = obj->findComponent<VisualComponent>();

		 glm::vec3 offset(0.0);
		 obj->addComponent(make_unique<ParticleComponent>(vulcanInstance.m_PhysicalDevice, vulcanInstance.m_Device, visualComp->m_ModelData, offset, particleRender.m_ParticleRendererData));
		 modelData = visualComp->m_ModelData;

		 sceneObjectManager.insert(move(obj));
	 }

	 {
		 unique_ptr<SceneObject> obj = sceneObjectFactory.createSceneObjectFromFile("./../Models/test11/bathroomFloor.obj");

		 glm::mat4& mat = obj->getMatrix();
		 mat[0][0] = mat[1][1] = mat[2][2] = 0.6f;
		 mat[3][3] = 1.0f;

		 mat[3][1] = -95.0f;

		 sceneObjectManager.insert(move(obj));
	 }

	 /*
	 {
		 unique_ptr<SceneObject> obj = sceneContext.m_SceneObjectFactory->createSceneObjectFromFile("./../Models/test9/edgy.obj");

		 glm::mat4& mat = obj->getMatrix();
		 mat[0][0] = mat[1][1] = mat[2][2] = 1.0f;
		 mat[3][3] = 1.0f;

		 mat[3][0] = 200.0f;

		 sceneContext.m_SceneObjectManager.insert(move(obj));
	 }

	 {
		 unique_ptr<SceneObject> obj = sceneContext.m_SceneObjectFactory->createSceneObjectFromFile("./../Models/test10/model.obj");

		 glm::mat4& mat = obj->getMatrix();
		 mat[0][0] = mat[1][1] = mat[2][2] = 50.0f;
		 mat[3][3] = 1.0f;

		 mat[3][0] = 300.0f;

		 sceneContext.m_SceneObjectManager.insert(move(obj));
	 }

	 {
		 unique_ptr<SceneObject> obj = sceneContext.m_SceneObjectFactory->createSceneObjectFromFile("./../Models/test8/Model.obj");

		 glm::mat4& mat = obj->getMatrix();
		 mat[0][0] = mat[1][1] = mat[2][2] = 100.0f;
		 mat[3][3] = 1.0f;

		 mat[3][0] = 500.0f;

		 sceneContext.m_SceneObjectManager.insert(move(obj));
	 }
	 */



	 window.setKeyCallback(key_callback);
	 window.setMouseMoveCallback(cursor_position_callback);
	 window.setMouseButtonCallback(mouse_button_callback);


	 vector<ParticleComponent*> particleComponents;
	 sceneObjectManager.enumerate([&](SceneObject* sceneObj)
		 {
			 ParticleComponent* particleComp = sceneObj->findComponent<ParticleComponent>();
			 if (particleComp)
			 {
				 particleComponents.push_back(particleComp);
			 }
		 });

	 particleRender.recordComputeCommand(particleComponents);


	 volatile static bool restart = false;
	 while (!window.shouldClose())
	 {
		 window.pollEvents();


		 if (keyDown[' '])
		 {
			 glm::vec3 offset((0.5f * (-500 + rand() % 1000) / 500.0f), 0.5f * ((rand() % 1000) / 1000.0f), 1.0f * (-500 + rand() % 1000) / 500.0f);

			 for (auto& it : particleComponents)
			 {
				 it->reset(*particleComponents[0], modelData, offset);
			 }
		 }


		 particleRender.submitComputeCommand();

		 vulcanInstance.drawFrame([&particleRender, &sceneObjectManager](VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer, VkImage image)
			 {

				 VkCommandBufferBeginInfo beginInfo = {};
				 beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				 beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				 beginInfo.pInheritanceInfo = nullptr; // Optional

				 auto res = vkBeginCommandBuffer(commandBuffer, &beginInfo);
				 assert(res == VK_SUCCESS);

				 { ///particles pass
					 VkRenderPassBeginInfo renderPassInfo = {};
					 renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					 renderPassInfo.renderPass = particleRender.m_RenderPass;
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

					 sceneObjectManager.enumerate([&](SceneObject* sceneObj)
						 {
							 ParticleComponent const* particleComp = sceneObj->findComponent<ParticleComponent>();
							 if (particleComp)
							 {
								 glm::mat4 mvp = projectionMatrix * camera->getInvMatrix() * sceneObj->getMatrix();

								 for (auto& group : particleComp->m_ParticleGroups)
								 {
									 vkCmdPushConstants(commandBuffer, particleRender.m_ParticleRenderPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);

									 auto descriptorSet = group.m_DescriptorSet.getDescriptorSet();
									 vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, particleRender.m_ParticleRenderPipelineLayout, 0, 1, &descriptorSet, 0, NULL);
									 vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, particleRender.m_ParticleRenderPipeline);
									 vkCmdDraw(commandBuffer, group.m_Particles.count * 3, 1, 0, 0);
								 }
							 }
						 });

					 vkCmdEndRenderPass(commandBuffer);
				 }

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

 };

