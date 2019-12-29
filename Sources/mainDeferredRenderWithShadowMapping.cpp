
#include "mainDeferredRenderWithShadowMapping.h"

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


 void runDeferredRenderWithShadowMapping()
 {
	 Window window(resX, resY, "Vulkan");
	 VulcanInstance vulcanInstance(window.getWindow(), resX, resY);

	 SceneObjectFactory sceneObjectFactory(vulcanInstance.m_PhysicalDevice, vulcanInstance.m_Device, vulcanInstance.m_CommandPool, vulcanInstance.m_GraphicQueue);
	 ShadowRenderer shadowRender(vulcanInstance.m_Device, vulcanInstance.m_PhysicalDevice, vulcanInstance.m_SwapChainExtent);
	 DeferredRender deferredRender(vulcanInstance.m_Device, vulcanInstance.m_PhysicalDevice, vulcanInstance.m_SwapChainExtent, vulcanInstance.m_SwapChainImageFormat, shadowRender.m_ShadowAttachment.view, vulcanInstance.m_RenderPass, sceneObjectFactory.getMaterialManager()->getDescriptorSetLayout());

	 SceneContext sceneContext;
	 sceneContext.m_SceneDescription = make_unique<SceneDescription>(vulcanInstance.m_PhysicalDevice, vulcanInstance.m_Device, deferredRender.m_1stPassDescriptorSetLayout2);
	 sceneContext.m_GraphicsPipeline = deferredRender.m_1stPassPipeline;
	 sceneContext.m_PipelineLayout = deferredRender.m_1stPassPipelineLayout;
	 sceneContext.m_Camera.setPos(glm::vec3(0.0f, 0.0f, 200.0f));
	 sceneContext.m_Camera.setDir(glm::vec3(0.0f, 0.0f, -1.0f));
	 sceneContext.m_Lights.resize(2);
	 sceneContext.m_Lights[0].setPos(glm::vec3(0.0f, 200.0f, 200.0f));
	 sceneContext.m_Lights[0].setDir(glm::vec3(0.0f, -1.0f, -1.0f));
	 sceneContext.m_Lights[1].setPos(glm::vec3(0.0f, 250.0f, 300.0f));
	 sceneContext.m_Lights[1].setDir(glm::vec3(0.0f, -1.0f, -1.0f));
	 sceneContext.m_ProjectionMatrix = VulkanHelpers::preparePerspectiveProjectionMatrix((float)resX / resY, 60, 1.0f, 10000.0f);

	 camera = &sceneContext.m_Camera;

	 ModelDataSharedPtr modelData;
	 {
		 unique_ptr<SceneObject> obj = sceneObjectFactory.createSceneObjectFromFile("./../Models/test7/Model.obj");
		 glm::mat4& mat = obj->getMatrix();
		 mat[0][0] = mat[1][1] = mat[2][2] = 100.0f;
		 mat[3][3] = 1.0f;
		 mat[3][0] = 0.0f;

		 sceneContext.m_SceneObjectManager.insert(move(obj));
	 }

	 {
		 unique_ptr<SceneObject> obj = sceneObjectFactory.createSceneObjectFromFile("./../Models/test11/bathroomFloor.obj");

		 glm::mat4& mat = obj->getMatrix();
		 mat[0][0] = mat[1][1] = mat[2][2] = 0.6f;
		 mat[3][3] = 1.0f;
		 mat[3][1] = -95.0f;

		 sceneContext.m_SceneObjectManager.insert(move(obj));
	 }

	 window.setKeyCallback(key_callback);
	 window.setMouseMoveCallback(cursor_position_callback);
	 window.setMouseButtonCallback(mouse_button_callback);

	 volatile static bool restart = false;
	 while (!window.shouldClose())
	 {
		 window.pollEvents();

		 vulcanInstance.drawFrame([&sceneContext, &deferredRender, &shadowRender](VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer, VkImage image)
			 {
				 VkCommandBufferBeginInfo beginInfo = {};
				 beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				 beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				 beginInfo.pInheritanceInfo = nullptr; // Optional

				 auto res = vkBeginCommandBuffer(commandBuffer, &beginInfo);
				 assert(res == VK_SUCCESS);

				 { //Deffered renderer 1st pass to fill Gbuffers with values
					 array<VkClearValue, 5> clearValues = {};
					 clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
					 clearValues[1].color = { 0.0f, 0.0f, 0.0f, 0.0f };
					 clearValues[2].color = { 0.0f, 0.0f, 0.0f, 0.0f };
					 clearValues[3].color = { 0.0f, 0.0f, 0.0f, 0.0f };
					 clearValues[4].depthStencil = { 1.0f, 0 };

					 VkRenderPassBeginInfo renderPassInfo = {};
					 renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

					 renderPassInfo.renderPass = deferredRender.m_1stRenderPass;
					 renderPassInfo.framebuffer = deferredRender.m_1stFramebuffer;

					 renderPassInfo.renderArea.offset = { 0, 0 };
					 renderPassInfo.renderArea.extent = deferredRender.m_Extent;
					 renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
					 renderPassInfo.pClearValues = &clearValues[0];

					 vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

					 sceneContext.recordCommandBuffer(commandBuffer);

					 vkCmdEndRenderPass(commandBuffer);
				 }


				 for (size_t i = 0; i < sceneContext.m_Lights.size(); ++i)
				 {

					 { //Generate shadow map
						 array<VkClearValue, 1> clearValues = {};
						 clearValues[0].depthStencil = { 1.0f, 0 };

						 VkRenderPassBeginInfo renderPassInfo = {};
						 renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
						 renderPassInfo.renderPass = shadowRender.m_ShadowRenderPass;
						 renderPassInfo.framebuffer = shadowRender.m_ShadowFramebuffer;
						 renderPassInfo.renderArea.offset = { 0, 0 };
						 renderPassInfo.renderArea.extent = shadowRender.m_Extent;
						 renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
						 renderPassInfo.pClearValues = &clearValues[0];

						 vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

						 vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowRender.m_ShadowGraphicPipeline);

						 VkDescriptorSet  descSet = shadowRender.m_ShadowDescriptorSet->getDescriptorSet();
						 vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowRender.m_ShadowPipelineLayout, 0, 1, &descSet, 0, nullptr);

						 sceneContext.m_SceneObjectManager.enumerate([&](SceneObject* sceneObj)
							 {
								 array<glm::mat4, 3> matrices = { sceneContext.m_ProjectionMatrix, sceneContext.m_Lights[i].getInvMatrix(), sceneObj->getMatrix() };
								 vkCmdPushConstants(commandBuffer, shadowRender.m_ShadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(matrices), &matrices[0]);

								 VisualComponent const* visualComp = sceneObj->findComponent<VisualComponent>();
								 if (visualComp)
								 {
									 for (size_t i = 0; i < visualComp->m_ModelData->meshes.size(); ++i)
									 {
										 VkDeviceSize offsets[] = { 0 };
										 vkCmdBindVertexBuffers(commandBuffer, 0, 1, &visualComp->m_ModelData->meshes[i].vertexBuffer.buffer, offsets);

										 vkCmdBindIndexBuffer(commandBuffer, visualComp->m_ModelData->meshes[i].indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
										 vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(visualComp->m_ModelData->meshes[i].indexBuffer.count), 1, 0, 0, 0);
									 }
								 }

							 });

						 vkCmdEndRenderPass(commandBuffer);
					 }


					 { //Use shadow map and Gbuffer values to calculate illumination
						 array<VkClearValue, 2> clearValues = {};
						 clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
						 clearValues[1].depthStencil = { 1.0f, 0 };

						 VkClearAttachment clearAtt;
						 clearAtt.colorAttachment = 0;
						 clearAtt.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						 clearAtt.clearValue = clearValues[0];

						 VkClearRect rect;
						 rect.baseArrayLayer = 0;
						 rect.layerCount = 1;
						 rect.rect.offset = { 0,0 };
						 rect.rect.extent = { 1024,1024 };

						 //vkCmdClearColorImage(commandBuffer, 1, clearValues[0]
						 VkRenderPassBeginInfo renderPassInfo = {};
						 renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
						 renderPassInfo.renderPass = deferredRender.m_DstRenderPass;
						 renderPassInfo.framebuffer = frameBuffer;
						 renderPassInfo.renderArea.offset = { 0, 0 };
						 renderPassInfo.renderArea.extent = deferredRender.m_Extent;
						 renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
						 renderPassInfo.pClearValues = &clearValues[0];

						 vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);


						 //==================
						 LightParamUBO lightParam;

						 lightParam.viewProj = sceneContext.m_ProjectionMatrix * sceneContext.m_Lights[i].getInvMatrix() * sceneContext.m_Camera.getMatrix();
						 lightParam.viewSpacePos = glm::vec3(sceneContext.m_Camera.getInvMatrix() * sceneContext.m_Lights[i].getMatrix()[3]);
						 lightParam.color = glm::vec3(1000000.0f);

						 vkCmdPushConstants(commandBuffer, deferredRender.m_2ndPassPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(lightParam), &lightParam);
						 deferredRender.m_lightParamBuffer->updateBuffer(&lightParam);
						 //==================

						 if (i == 0)
						 {
							 vkCmdClearAttachments(commandBuffer, 1, &clearAtt, 1, &rect);
						 }


						 VkDescriptorSet  descSet = deferredRender.m_2ndPassDescriptorSet->getDescriptorSet();
						 vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredRender.m_2ndPassPipelineLayout, 0, 1, &descSet, 0, NULL);
						 vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredRender.m_2ndPassPipeline);

						 vkCmdDraw(commandBuffer, 6, 1, 0, 0);

						 vkCmdEndRenderPass(commandBuffer);
					 }
				 }
				 res = vkEndCommandBuffer(commandBuffer);
				 assert(res == VK_SUCCESS);

			 });


		 const float speed = 10.0f;
		 if (keyDown['E']) sceneContext.m_Camera.move(0.0f, 0.0f, speed);
		 if (keyDown['Q']) sceneContext.m_Camera.move(0.0f, 0.0f, -speed);
		 if (keyDown['W']) sceneContext.m_Camera.move(speed, 0.0f, 0.0f);
		 if (keyDown['S']) sceneContext.m_Camera.move(-speed, 0.0f, 0.0f);
		 if (keyDown['A']) sceneContext.m_Camera.move(0.0f, -speed, 0.0f);
		 if (keyDown['D']) sceneContext.m_Camera.move(0.0f, speed, 0.0f);




		 vector<glm::quat> rot(2);

		 rot[0] = glm::angleAxis(glm::radians(0.05f), glm::vec3(0.f, 1.f, 0.f));
		 rot[1] = glm::angleAxis(glm::radians(-0.1f), glm::vec3(0.f, 1.f, 0.f));

		 for (size_t i = 0; i < sceneContext.m_Lights.size(); ++i)
		 {
			 glm::vec3 pos = sceneContext.m_Lights[i].getMatrix()[3];
			 pos = pos * rot[i];

			 glm::vec3 dir = glm::normalize(-pos);
			 sceneContext.m_Lights[i].setPos(pos);
			 sceneContext.m_Lights[i].setDir(dir);
		 }

	 }

	 sceneContext.m_SceneObjectManager.clear();

 };

