#pragma once

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


class SceneObjectFactory
{
public:
	SceneObjectFactory(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool  commandPool, VkQueue graphicQueue);
	unique_ptr<SceneObject> createSceneObjectFromFile(string const& path);
	MaterialManager* getMaterialManager() const;

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

	SceneDescription(VkPhysicalDevice physicalDevice, VkDevice device, shared_ptr<DescriptorSetLayout> descriptorSetLayout);
};

struct SceneContext
{
	unique_ptr < SceneDescription> m_SceneDescription;
	SceneObjectManager m_SceneObjectManager;
	glm::mat4x4 m_ProjectionMatrix;
	Camera m_Camera;
	vector<Camera> m_Lights;

	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_GraphicsPipeline;

	void recordCommandBuffer(VkCommandBuffer commandBuffer);
};

extern array<bool, 128> keyDown;
extern bool mouseButtonLeftDown;
extern double mouseX, mouseY;
extern Camera* camera;
extern unsigned int resX;
extern unsigned int resY;

void key_callback(Window& window, int key, int scancode, int action, int mods);
void mouse_button_callback(Window& window, int button, int action, int mods);
void cursor_position_callback(Window& window, double xpos, double ypos);