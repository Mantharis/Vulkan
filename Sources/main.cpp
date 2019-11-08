

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

 using namespace std;




class SceneObjectFactory
{
public:
	SceneObjectFactory(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool  commandPool, VkQueue graphicQueue, shared_ptr <DescriptorSetLayout> descriptorSetLayout)
	{
		m_TextureManager = make_unique<TextureManager>(physicalDevice, device, commandPool, graphicQueue);
		m_MaterialManager = make_unique<MaterialManager>(*m_TextureManager, device, physicalDevice, move(descriptorSetLayout));
		m_ModelManager = make_unique<ModelManager>(physicalDevice, device, *m_MaterialManager);
	}

	unique_ptr<SceneObject> createSceneObjectFromFile(string const& path)
	{
		auto cityModel = m_ModelManager->loadModel(path);

		unique_ptr<SceneObject> obj = make_unique<SceneObject>();
		unique_ptr<VisualComponent> visualComp = make_unique<VisualComponent>(cityModel);
		obj->addComponent(move(visualComp));

		return obj;
	}

	MaterialManager* getMaterialManager() const
	{
		return &*m_MaterialManager;
	}

private:
	unique_ptr<TextureManager> m_TextureManager;
	unique_ptr<ModelManager> m_ModelManager;
	unique_ptr<MaterialManager> m_MaterialManager;
};


const unsigned int MAX_LIGHT_COUNT = 10;
struct SceneData
{
	alignas(16) glm::vec3 cameraPos = glm::vec3(1.0f, 0.0f, 0.0f);
	alignas(16) glm::vec3 lightPos = glm::vec3(0.0f, 590.0f, 0.0f);
	alignas(16) glm::vec3 lightColor;
	alignas(4) unsigned int lightCount;
};

struct SceneDescription
{
	SceneData m_Data;
	Buffer m_UniformBuffer;
	DescriptorSet m_DescriptorSet;

	SceneDescription(VkPhysicalDevice physicalDevice, VkDevice device, shared_ptr<DescriptorSetLayout> descriptorSetLayout)
		:m_UniformBuffer(physicalDevice, device, &m_Data, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),
		m_DescriptorSet(move(descriptorSetLayout))
		//m_DescriptorSet(device, descriptorSetLayout, vector<variant< VkDescriptorImageInfo, VkDescriptorBufferInfo>>(1, VkDescriptorBufferInfo{ m_UniformBuffer.buffer, 0, sizeof(SceneData)}) )
	{
	
		m_DescriptorSet.addBuffer("sceneBuffer", m_UniformBuffer);
		m_DescriptorSet.createDescriptorSet();
	}

};




struct SceneContext
{
	unique_ptr < SceneDescription> m_SceneDescription;
	unique_ptr<SceneObjectFactory> m_SceneObjectFactory;
	SceneObjectManager m_SceneObjectManager;
	glm::mat4x4 m_ProjectionMatrix;
	Camera m_Camera;
	Camera m_Light;

	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_GraphicsPipeline;

	void recordCommandBuffer(VkCommandBuffer commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

		//static glm::vec3 moveDir(1.0f, 3.0f, 2.0f);
		static glm::vec3 moveDir(0.0f, 0.0f, 0.0f);

		m_SceneDescription->m_Data.lightPos = m_Camera.getMatrix()[3]; // glm::vec3(0.0f, 0.0f, 100.0f);
		//m_SceneDescription->m_Data.lightPos += moveDir;

		if (abs(m_SceneDescription->m_Data.lightPos.x) > 500.0f)
			moveDir.x = -moveDir.x;
		if (abs(m_SceneDescription->m_Data.lightPos.z) > 500.0f)
			moveDir.z = -moveDir.z;

		if (m_SceneDescription->m_Data.lightPos.y > 1000.0f || m_SceneDescription->m_Data.lightPos.y < 100.0f)
			moveDir.y = -moveDir.y;


		m_SceneDescription->m_Data.lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
		m_SceneDescription->m_Data.lightCount = 1;
		m_SceneDescription->m_Data.cameraPos = m_Camera.getMatrix()[3];

		m_SceneDescription->m_UniformBuffer.updateBuffer(&m_SceneDescription->m_Data, 1);

		auto descriptorSet = m_SceneDescription->m_DescriptorSet.getDescriptorSet();
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 1, 1, &descriptorSet, 0, nullptr);

		m_SceneObjectManager.enumerate([&](SceneObject* sceneObj)
			{
				array<glm::mat4, 3> matrices = { m_ProjectionMatrix, m_Camera.getInvMatrix(), sceneObj->getMatrix() };
				vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(matrices), &matrices[0]);

				VisualComponent const* visualComp = sceneObj->findComponent<VisualComponent>();
				if (visualComp) visualComp->draw(commandBuffer, m_PipelineLayout);
			});
	}
};


array<bool, 128> keyDown;
bool mouseButtonLeftDown = false;
double mouseX, mouseY;
Camera* camera = nullptr;

size_t resX = 512;
size_t resY = 512;


void key_callback(Window& window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS) keyDown[key] = true;
	else if (action == GLFW_RELEASE) keyDown[key] = false;
}

void mouse_button_callback(Window& window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{

		mouseButtonLeftDown = true;
		glfwSetCursorPos(&window.getWindow(), resX / 2, resY / 2);
		glfwSetInputMode(&window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		mouseX = 400.0f;
		mouseY = 300.0f;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		mouseButtonLeftDown = false;
		glfwSetInputMode(&window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		glfwSetCursorPos(&window.getWindow(), resX / 2, resY / 2);
	}
}

void cursor_position_callback(Window& window, double xpos, double ypos)
{
	if (mouseButtonLeftDown)
	{
		float diffX = xpos - mouseX;
		float diffY = ypos - mouseY;

		mouseX = xpos;
		mouseY = ypos;

		camera->rotate( diffY * 0.01f, -diffX * 0.01f);
	}
}


int main() 
{
	Window window(resX, resY, "Vulkan");
	VulcanInstance vulcanInstance(window.getWindow(), resX, resY);


	//===============================
	VkRenderPass shadowRenderpass;
	VulkanHelpers::createRenderPass(shadowRenderpass, vulcanInstance.m_Device, VkFormat::VK_FORMAT_END_RANGE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, true, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

	uint32_t shadowTexSize = 512;
	AttachmentData shadowAttachment;

	VulkanHelpers::createAttachmnent(shadowAttachment, vulcanInstance.m_PhysicalDevice, vulcanInstance.m_Device, VK_FORMAT_D16_UNORM, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, { shadowTexSize, shadowTexSize });

	//FRAMEBUFFER
	vector<VkImageView> attachments;
	attachments.push_back(shadowAttachment.view);
	

	VkFramebuffer shadowFramebuffer;
	VulkanHelpers::createFramebuffer( shadowFramebuffer, shadowRenderpass, attachments, vulcanInstance.m_Device, { shadowTexSize, shadowTexSize });


	//DESCRIPTOR SET
	shared_ptr<DescriptorSetLayout> descriptorSetLayout = make_shared< DescriptorSetLayout>(vulcanInstance.m_Device);
	descriptorSetLayout->createDescriptorSetLayout();

	static shared_ptr< DescriptorSet> descriptorSet = make_shared< DescriptorSet>(descriptorSetLayout);
	descriptorSet->createDescriptorSet();

	VkDescriptorSet shadowDecriptorSet = descriptorSet->getDescriptorSet();

	//GRAPHIC PIPELINE
	ShaderSet shaderData;
	shaderData.vertexInputBindingDescription = Vertex::getBindingDescription();
	shaderData.vertexShaderPath = string("./../Shaders/shadowShaderVert.spv");
	shaderData.fragmentShaderPath = string("./../Shaders/shadowShaderFrag.spv");
	shaderData.pushConstant.resize(1);
	shaderData.pushConstant[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	shaderData.pushConstant[0].offset = 0;
	shaderData.pushConstant[0].size = sizeof(glm::mat4) * 3;
	shaderData.descriptorSetLayout.push_back(descriptorSet->getDescriptorSetlayout()->getLayout());
	
	for (auto& attribute : Vertex::getAttributeDescriptions())
		shaderData.vertexInputAttributeDescription.push_back(attribute);


	/*
	//GRAPHIC PIPELINE
	ShaderSet shaderData;
	shaderData.vertexInputBindingDescription = {};
	shaderData.vertexShaderPath = string("./../Shaders/shadowShaderVert.spv");
	shaderData.fragmentShaderPath = string("./../Shaders/shadowShaderFrag.spv");
	shaderData.descriptorSetLayout.push_back(descriptorSet->getDescriptorSetlayout()->getLayout());
	*/

	VkPipeline shadowGraphicPipeline;
	VkPipelineLayout shadowPipelineLayout;
	VulkanHelpers::createGraphicsPipeline(vulcanInstance.m_Device, shadowRenderpass, 0, { shadowTexSize, shadowTexSize }, shaderData, shadowPipelineLayout, shadowGraphicPipeline);

	//TODO
	DeferredRender::depth = shadowAttachment.view; // offscreenPass.depth.view;//  shadowAttachment.view;

	//===============================

	DeferredRender deferredRender(vulcanInstance.m_Device, vulcanInstance.m_PhysicalDevice, vulcanInstance.m_SwapChainExtent, vulcanInstance.m_SwapChainImageFormat);



	//============================================
	//============================================


	SceneContext sceneContext;

	sceneContext.m_SceneObjectFactory = make_unique<SceneObjectFactory>(vulcanInstance.m_PhysicalDevice, vulcanInstance.m_Device, vulcanInstance.m_CommandPool, vulcanInstance.m_GraphicQueue, deferredRender.m_1stPassDescriptorSetLayout);
	sceneContext.m_SceneDescription = make_unique<SceneDescription>(vulcanInstance.m_PhysicalDevice, vulcanInstance.m_Device, deferredRender.m_1stPassDescriptorSetLayout2);
	sceneContext.m_GraphicsPipeline = deferredRender.m_1stPassPipeline;
	sceneContext.m_GraphicsPipeline = deferredRender.m_1stPassPipeline;
	sceneContext.m_PipelineLayout = deferredRender.m_1stPassPipelineLayout;
	sceneContext.m_Camera.setPos(glm::vec3(0.0f, 0.0f, 200.0f));
	sceneContext.m_Camera.setDir(glm::vec3(0.0f, 0.0f, -1.0f));

	sceneContext.m_Light.setPos(glm::vec3(0.0f, 200.0f, 200.0f));
	sceneContext.m_Light.setDir(glm::vec3(0.0f, -1.0f, -1.0f));
	
	sceneContext.m_ProjectionMatrix = VulkanHelpers::preparePerspectiveProjectionMatrix((float)resX / resY, 60, 1.0f, 10000.0f);

	camera = &sceneContext.m_Camera;

	/*
	{
		unique_ptr<SceneObject> obj = sceneContext.m_SceneObjectFactory->createSceneObjectFromFile("./../Models/test4/model.obj");

		glm::mat4& mat = obj->getMatrix();
		mat[0][0] = mat[1][1] = mat[2][2] = 100.0f;
		mat[3][3] = 1.0f;

		sceneContext.m_SceneObjectManager.insert(move(obj));
	}
	*/

	{
		unique_ptr<SceneObject> obj = sceneContext.m_SceneObjectFactory->createSceneObjectFromFile("./../Models/test7/Model.obj");

		glm::mat4& mat = obj->getMatrix();
		mat[0][0] = mat[1][1] = mat[2][2] = 100.0f;
		mat[3][3] = 1.0f;

		mat[3][0] = 0.0f;

		sceneContext.m_SceneObjectManager.insert(move(obj));
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

	while (!window.shouldClose())
	{

		window.pollEvents();



		vulcanInstance.drawFrame([&sceneContext, &deferredRender, 
			 shadowRenderpass, shadowFramebuffer, shadowTexSize, shadowGraphicPipeline, shadowPipelineLayout, shadowDecriptorSet](VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer)
			{
				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				beginInfo.pInheritanceInfo = nullptr; // Optional

				
				auto res = vkBeginCommandBuffer(commandBuffer, &beginInfo);
				assert(res == VK_SUCCESS);

				
				{
					array<VkClearValue, 1> clearValues = {};
					clearValues[0].depthStencil = { 1.0f, 0 };
				
					VkRenderPassBeginInfo renderPassInfo = {};
					renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					renderPassInfo.renderPass =  shadowRenderpass;
					renderPassInfo.framebuffer =   shadowFramebuffer;
					renderPassInfo.renderArea.offset = { 0, 0 };
					renderPassInfo.renderArea.extent = { shadowTexSize, shadowTexSize };
					renderPassInfo.clearValueCount = clearValues.size();
					renderPassInfo.pClearValues = &clearValues[0];
				
					vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
				
					vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowGraphicPipeline);
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout, 0, 1, &shadowDecriptorSet, 0, nullptr);
					
					
					sceneContext.m_SceneObjectManager.enumerate([&](SceneObject* sceneObj)
						{
							array<glm::mat4, 3> matrices = { sceneContext.m_ProjectionMatrix, sceneContext.m_Light.getInvMatrix(), sceneObj->getMatrix() };
							vkCmdPushConstants(commandBuffer, shadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(matrices), &matrices[0]);
					
							VisualComponent const* visualComp = sceneObj->findComponent<VisualComponent>();
							if (visualComp)
							{
								for (size_t i = 0; i < visualComp->m_ModelData->meshes.size(); ++i)
								{
									VkDeviceSize offsets[] = { 0 };
									vkCmdBindVertexBuffers(commandBuffer, 0, 1, &visualComp->m_ModelData->meshes[i].vertexBufffer, offsets);
					
									vkCmdBindIndexBuffer(commandBuffer, visualComp->m_ModelData->meshes[i].indexBuffer, 0, VK_INDEX_TYPE_UINT32);
									vkCmdDrawIndexed(commandBuffer, visualComp->m_ModelData->meshes[i].indexBufferSize, 1, 0, 0, 0);
								}
							}
							
						});
						
					vkCmdEndRenderPass(commandBuffer);
				}
				
				
				{
					array<VkClearValue, 5> clearValues = {};
					clearValues[0].color ={0.0f, 0.0f, 0.0f, 1.0f };
					clearValues[1].color ={0.0f, 0.0f, 0.0f, 1.0f };
					clearValues[2].color ={0.0f, 0.0f, 0.0f, 1.0f };
					clearValues[3].color ={0.0f, 0.0f, 0.0f, 1.0f };
					clearValues[4].depthStencil = { 1.0f, 0 };

					VkRenderPassBeginInfo renderPassInfo = {};
					renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					
					renderPassInfo.renderPass = deferredRender.m_1stRenderPass;
					renderPassInfo.framebuffer = deferredRender.m_1stFramebuffer;

					renderPassInfo.renderArea.offset = { 0, 0 };
					renderPassInfo.renderArea.extent = deferredRender.m_Extent;
					renderPassInfo.clearValueCount = clearValues.size();
					renderPassInfo.pClearValues = &clearValues[0];

					vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
					
					sceneContext.recordCommandBuffer(commandBuffer);

					vkCmdEndRenderPass(commandBuffer);
				}

			
				{
					array<VkClearValue, 2> clearValues = {};
					clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f };
					clearValues[1].depthStencil = { 1.0f, 0 };


					VkRenderPassBeginInfo renderPassInfo = {};
					renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					renderPassInfo.renderPass = deferredRender.m_2ndRenderPass;
					renderPassInfo.framebuffer = frameBuffer;
					renderPassInfo.renderArea.offset = { 0, 0 };
					renderPassInfo.renderArea.extent = deferredRender.m_Extent;
					renderPassInfo.clearValueCount = clearValues.size();
					renderPassInfo.pClearValues = &clearValues[0];


					//==================
					LightParamUBO lightParam;

				

					lightParam.viewProj = sceneContext.m_ProjectionMatrix * sceneContext.m_Light.getInvMatrix() * sceneContext.m_Camera.getMatrix();
					lightParam.viewSpacePos = glm::vec3(  sceneContext.m_Camera.getInvMatrix() * sceneContext.m_Light.getMatrix()[3]);
					lightParam.color = glm::vec3(100000.0f);

					deferredRender.m_lightParamBuffer->updateBuffer(&lightParam, 1);
					//==================

					vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

					VkDescriptorSet  descSet = deferredRender.m_2ndPassDescriptorSet->getDescriptorSet();
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredRender.m_2ndPassPipelineLayout, 0, 1, &descSet, 0, NULL);
					vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredRender.m_2ndPassPipeline);
					vkCmdDraw(commandBuffer, 6, 1, 0, 0);

					vkCmdEndRenderPass(commandBuffer);
				}

				res = vkEndCommandBuffer(commandBuffer);
				assert(res == VK_SUCCESS);

			});


		const float speed = 10.0f;
		if (keyDown['E']) sceneContext.m_Camera.move( 0.0f, 0.0f, speed);
		if (keyDown['Q']) sceneContext.m_Camera.move(0.0f, 0.0f, -speed);
		if (keyDown['W']) sceneContext.m_Camera.move(speed, 0.0f, 0.0f);
		if (keyDown['S']) sceneContext.m_Camera.move(-speed, 0.0f, 0.0f);
		if (keyDown['A']) sceneContext.m_Camera.move(0.0f, -speed, 0.0f);
		if (keyDown['D']) sceneContext.m_Camera.move(0.0f, speed, 0.0f);

	}


	//TODO
	//vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
	//vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
	
	sceneContext.m_SceneObjectManager.clear();

	return 0;
};