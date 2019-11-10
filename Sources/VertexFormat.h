#pragma once

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <vulkan/vulkan.h>

#include <array>
using namespace std;

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 bitangent;

	static VkVertexInputBindingDescription getBindingDescription();
	static array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions();
};
