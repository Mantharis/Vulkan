#include "mainCommon.h"

array<bool, 128> keyDown;
bool mouseButtonLeftDown = false;
double mouseX, mouseY;
Camera* camera;

unsigned int resX = 1024;
unsigned int resY = 1024;

SceneObjectFactory::SceneObjectFactory(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool  commandPool, VkQueue graphicQueue)
{
	m_TextureManager = make_unique<TextureManager>(physicalDevice, device, commandPool, graphicQueue);
	m_MaterialManager = make_unique<MaterialManager>(*m_TextureManager, device, physicalDevice);
	m_ModelManager = make_unique<ModelManager>(physicalDevice, device, *m_MaterialManager);
}

unique_ptr<SceneObject> SceneObjectFactory::createSceneObjectFromFile(string const& path)
{
	auto cityModel = m_ModelManager->loadModel(path);

	unique_ptr<SceneObject> obj = make_unique<SceneObject>();
	unique_ptr<VisualComponent> visualComp = make_unique<VisualComponent>(cityModel);
	obj->addComponent(move(visualComp));

	return obj;
}

MaterialManager* SceneObjectFactory::getMaterialManager() const
{
	return &*m_MaterialManager;
}

SceneDescription::SceneDescription(VkPhysicalDevice physicalDevice, VkDevice device, shared_ptr<DescriptorSetLayout> descriptorSetLayout)
	:m_UniformBuffer(physicalDevice, device, &m_Data, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),
	m_DescriptorSet(move(descriptorSetLayout))
	//m_DescriptorSet(device, descriptorSetLayout, vector<variant< VkDescriptorImageInfo, VkDescriptorBufferInfo>>(1, VkDescriptorBufferInfo{ m_UniformBuffer.buffer, 0, sizeof(SceneData)}) )
{
	m_DescriptorSet.createDescriptorSet();
	m_DescriptorSet.setBuffer("sceneBuffer", m_UniformBuffer);
	
}

void SceneContext::recordCommandBuffer(VkCommandBuffer commandBuffer)
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

	m_SceneDescription->m_UniformBuffer.updateBuffer(&m_SceneDescription->m_Data);

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
		glfwSetCursorPos(&window.getWindow(), static_cast<double>(resX / 2), static_cast<double>(resY / 2));
		glfwSetInputMode(&window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		mouseX = 400.0f;
		mouseY = 300.0f;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		mouseButtonLeftDown = false;
		glfwSetInputMode(&window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		glfwSetCursorPos(&window.getWindow(), static_cast<double>(resX / 2), static_cast<double>(resY / 2));
	}
}

void cursor_position_callback(Window& window, double xpos, double ypos)
{
	if (mouseButtonLeftDown)
	{
		float diffX = static_cast<float>(xpos - mouseX);
		float diffY = static_cast<float>(ypos - mouseY);

		mouseX = xpos;
		mouseY = ypos;

		camera->rotate( -diffY * 0.01f, -diffX * 0.01f);
	}
}