

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

 using namespace std;

 float const Pi = 3.14159265358979323846f;

 float deg2Rad(float rad)
 {
	 return rad * 2.0f * Pi / 360.0f;
 }

 glm::mat4 preparePerspectiveProjectionMatrix(float aspect_ratio, float field_of_view, float near_plane, float far_plane) 
 {
	 float f = 1.0f / tan(deg2Rad(0.5f * field_of_view));

	 glm::mat4 projectionMatrix;
	 projectionMatrix[0] = glm::vec4(f / aspect_ratio, 0.0f, 0.0f, 0.0f);
	 projectionMatrix[1] = glm::vec4(0.0f, -f, 0.0f, 0.0f);
	 projectionMatrix[2] = glm::vec4(0.0f, 0.0f, far_plane / (near_plane - far_plane), -1.0f);
	 projectionMatrix[3] = glm::vec4(f / aspect_ratio, 0.0f, (near_plane * far_plane) / (near_plane - far_plane), 0.0f);

	 return projectionMatrix;
 }


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

size_t resX = 1280;
size_t resY = 1024;


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


class DefferedRender
{

public:
	DefferedRender(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent, VkFormat swapChainImageFormat) : m_Device(device), m_PhysicalDevice(physicalDevice), m_Extent(extent), m_SwapChainImageFormat(swapChainImageFormat)
	{
		create1stPass();
		create2ndPass();

	}

private:

	void create1stPass()
	{
		m_1stPassDescriptorSetLayout = make_shared<DescriptorSetLayout>(m_Device);
		m_1stPassDescriptorSetLayout->addDescriptor("materialBuffer", 0, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		m_1stPassDescriptorSetLayout->addDescriptor("diffuseTex", 1, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_1stPassDescriptorSetLayout->addDescriptor("specularTex", 2, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_1stPassDescriptorSetLayout->addDescriptor("normalTex", 3, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_1stPassDescriptorSetLayout->addDescriptor("specularHighlightTex", 4, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_1stPassDescriptorSetLayout->addDescriptor("aoTex", 5, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_1stPassDescriptorSetLayout->createDescriptorSetLayout();

		//RENDER PASS
		VulkanHelpers::createRenderPass(m_1stRenderPass, m_Device, s_ImageFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, s_ColorAttachmentCnt, true);

		//COLOR ATTACHMENTS
		m_1stColorAttachments.resize(s_ColorAttachmentCnt);

		for (auto& it : m_1stColorAttachments)
			VulkanHelpers::createAttachmnent(it, m_PhysicalDevice, m_Device, s_ImageFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, m_Extent);

		//DEPTH ATTACHMENT
		VulkanHelpers::createAttachmnent(m_1stDepthAttachment, m_PhysicalDevice, m_Device, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, m_Extent);

		//FRAMEBUFFER
		vector<VkImageView> attachments;
		
		for (auto& it : m_1stColorAttachments)
			attachments.push_back(it.view);

		attachments.push_back(m_1stDepthAttachment.view);

		VulkanHelpers::createFramebuffer(m_1stFramebuffer, m_1stRenderPass, attachments, m_Device, m_Extent);


		m_1stPassDescriptorSetLayout2 = make_shared<DescriptorSetLayout>(m_Device);
		m_1stPassDescriptorSetLayout2->addDescriptor("sceneBuffer", 0, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		m_1stPassDescriptorSetLayout2->createDescriptorSetLayout();


		ShaderSet shaderData;

		for (auto& attribute : Vertex::getAttributeDescriptions())
			shaderData.vertexInputAttributeDescription.push_back(attribute);

		shaderData.vertexInputBindingDescription = Vertex::getBindingDescription();
		shaderData.vertexShaderPath = string("./../Shaders/vert.spv");
		shaderData.fragmentShaderPath = string("./../Shaders/defferedShader1stPass.spv");
		shaderData.pushConstant.resize(1);
		shaderData.pushConstant[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		shaderData.pushConstant[0].offset = 0;
		shaderData.pushConstant[0].size = sizeof(glm::mat4) * 3;
		shaderData.descriptorSetLayout.push_back(m_1stPassDescriptorSetLayout->getLayout());
		shaderData.descriptorSetLayout.push_back(m_1stPassDescriptorSetLayout2->getLayout());

		VulkanHelpers::createGraphicsPipeline(m_Device, m_1stRenderPass, 4, m_Extent, shaderData, m_1stPassPipelineLayout, m_1stPassPipeline);

	}

	void create2ndPass()
	{
		VulkanHelpers::createRenderPass(m_2ndRenderPass, m_Device, m_SwapChainImageFormat, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1, true);

		//DESCRIPTOR SET
		shared_ptr<DescriptorSetLayout> descriptorSetLayout = make_shared< DescriptorSetLayout>(m_Device);
		descriptorSetLayout->addDescriptor("metalicRoughnessAo", 3, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorSetLayout->addDescriptor("normal", 2, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorSetLayout->addDescriptor("albedo", 1, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorSetLayout->addDescriptor("pos", 0, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorSetLayout->createDescriptorSetLayout();

		m_2ndPassDescriptorSet = make_shared< DescriptorSet>(descriptorSetLayout);

		m_Sampler = shared_ptr<const Sampler>(new Sampler(m_Device));

		m_2ndPassDescriptorSet->addSampler("albedo", m_1stColorAttachments[0].view, m_Sampler->m_Sampler);
		m_2ndPassDescriptorSet->addSampler("pos", m_1stColorAttachments[1].view, m_Sampler->m_Sampler);
		m_2ndPassDescriptorSet->addSampler("normal", m_1stColorAttachments[2].view, m_Sampler->m_Sampler);
		m_2ndPassDescriptorSet->addSampler("metalicRoughnessAo", m_1stColorAttachments[3].view, m_Sampler->m_Sampler);
		m_2ndPassDescriptorSet->createDescriptorSet();


		//GRAPHIC PIPELINE
		ShaderSet shaderData;
		shaderData.vertexInputBindingDescription = {};
		shaderData.vertexShaderPath = string("./../Shaders/defferedShader2ndPassVert.spv");
		shaderData.fragmentShaderPath = string("./../Shaders/defferedShader2ndPassFrag.spv");
		shaderData.descriptorSetLayout.push_back(m_2ndPassDescriptorSet->getDescriptorSetlayout()->getLayout());

		VulkanHelpers::createGraphicsPipeline(m_Device, m_2ndRenderPass, 1, m_Extent, shaderData, m_2ndPassPipelineLayout, m_2ndPassPipeline);
	}

	static const int s_ColorAttachmentCnt = 4;
	static const VkFormat s_ImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

public:
	VkExtent2D m_Extent;
	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice;
	VkFormat m_SwapChainImageFormat;

	shared_ptr<DescriptorSetLayout> m_1stPassDescriptorSetLayout;
	shared_ptr<DescriptorSetLayout> m_1stPassDescriptorSetLayout2;

	VkRenderPass m_1stRenderPass;
	VkFramebuffer m_1stFramebuffer;
	vector<AttachmentData> m_1stColorAttachments;
	AttachmentData m_1stDepthAttachment;
	VkPipelineLayout m_1stPassPipelineLayout;
	VkPipeline m_1stPassPipeline;


	VkRenderPass m_2ndRenderPass;

	shared_ptr<DescriptorSet> m_2ndPassDescriptorSet;
	shared_ptr<const Sampler> m_Sampler;
	VkPipelineLayout m_2ndPassPipelineLayout;
	VkPipeline m_2ndPassPipeline;
};

int main() 
{
	
	Window window(resX, resY, "Vulkan");
	VulcanInstance vulcanInstance(window.getWindow(), resX, resY);


	DefferedRender defferedRender(vulcanInstance.m_Device, vulcanInstance.m_PhysicalDevice, vulcanInstance.m_SwapChainExtent, vulcanInstance.m_SwapChainImageFormat);



	//============================================
	//============================================


	SceneContext sceneContext;

	sceneContext.m_SceneObjectFactory = make_unique<SceneObjectFactory>(vulcanInstance.m_PhysicalDevice, vulcanInstance.m_Device, vulcanInstance.m_CommandPool, vulcanInstance.m_GraphicQueue, defferedRender.m_1stPassDescriptorSetLayout);
	sceneContext.m_SceneDescription = make_unique<SceneDescription>(vulcanInstance.m_PhysicalDevice, vulcanInstance.m_Device, defferedRender.m_1stPassDescriptorSetLayout2);
	sceneContext.m_GraphicsPipeline = defferedRender.m_1stPassPipeline;
	sceneContext.m_GraphicsPipeline = defferedRender.m_1stPassPipeline;
	sceneContext.m_PipelineLayout = defferedRender.m_1stPassPipelineLayout;
	sceneContext.m_Camera.setPos(glm::vec3(0.0f, 100.0f, 300.0f));
	sceneContext.m_Camera.setDir(glm::vec3(0.0f, 0.0f, -1.0f));
	sceneContext.m_ProjectionMatrix = preparePerspectiveProjectionMatrix((float)resX / resY, 60, 1.0f, 10000.0f);


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

		mat[3][0] = 100.0f;

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

		vulcanInstance.drawFrame([&sceneContext, &defferedRender](VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer)
			{
				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				beginInfo.pInheritanceInfo = nullptr; // Optional

				auto res = vkBeginCommandBuffer(commandBuffer, &beginInfo);
				assert(res == VK_SUCCESS);

				
				{
					array<VkClearValue, 5> clearValues = {};
					clearValues[0].color = { 72.0f / 255.0f, 201.0f / 255.0f, 176.0f / 255.0f, 1.0f };
					clearValues[1].color = { 72.0f / 255.0f, 201.0f / 255.0f, 176.0f / 255.0f, 1.0f };
					clearValues[2].color = { 72.0f / 255.0f, 201.0f / 255.0f, 176.0f / 255.0f, 1.0f };
					clearValues[3].color = { 72.0f / 255.0f, 201.0f / 255.0f, 176.0f / 255.0f, 1.0f };
					clearValues[4].depthStencil = { 1.0f, 0 };

					VkRenderPassBeginInfo renderPassInfo = {};
					renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					
					renderPassInfo.renderPass = defferedRender.m_1stRenderPass;
					renderPassInfo.framebuffer = defferedRender.m_1stFramebuffer;

					renderPassInfo.renderArea.offset = { 0, 0 };
					renderPassInfo.renderArea.extent = defferedRender.m_Extent;
					renderPassInfo.clearValueCount = clearValues.size();
					renderPassInfo.pClearValues = &clearValues[0];

					vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
					
					sceneContext.recordCommandBuffer(commandBuffer);

					vkCmdEndRenderPass(commandBuffer);
				}

			
				{
					array<VkClearValue, 5> clearValues = {};
					clearValues[0].color = { 72.0f / 255.0f, 201.0f / 255.0f, 176.0f / 255.0f, 1.0f };
					clearValues[1].depthStencil = { 1.0f, 0 };


					VkRenderPassBeginInfo renderPassInfo = {};
					renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					renderPassInfo.renderPass = defferedRender.m_2ndRenderPass;
					renderPassInfo.framebuffer = frameBuffer;
					renderPassInfo.renderArea.offset = { 0, 0 };
					renderPassInfo.renderArea.extent = defferedRender.m_Extent;
					renderPassInfo.clearValueCount = clearValues.size();
					renderPassInfo.pClearValues = &clearValues[0];

					vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

					VkDescriptorSet  descSet = defferedRender.m_2ndPassDescriptorSet->getDescriptorSet();
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, defferedRender.m_2ndPassPipelineLayout, 0, 1, &descSet, 0, NULL);
					vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, defferedRender.m_2ndPassPipeline);
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