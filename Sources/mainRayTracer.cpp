
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

 template <class T>
 class IdxManager
 {
 private:
	 vector<T> m_Data;
	 unordered_map<string, int> m_NameToDataIdx;
 public:

	 int add(string const& name, T const& data)
	 {
		 auto it = m_NameToDataIdx.find(name);
		 if (it == m_NameToDataIdx.end())
		 {
			 m_Data.push_back(data);
			 m_NameToDataIdx[name] = m_Data.size()-1;
			 return m_Data.size();
		 }
		 else
		 {
			 return it->second;
		 }
	 }

	 int find(string const& name) const
	 {
		 auto it = m_NameToDataIdx.find(name);
		 if (it == m_NameToDataIdx.end())
		 {
			 return -1;
		 }
		 else
		 {
			 return it->second;
		 }
	 }

	 vector<T> const& getDataArray() const
	 {
		 return m_Data;
	 }
 };

 using TextureHelper = IdxManager< VkImageView>;
 using MaterialHelper = IdxManager< Material>;


void runRayTracing()
{
	camera = new Camera();
	camera->setPos(glm::vec3(0.0f, 0.0f, 200.0f));
	camera->setDir(glm::vec3(0.0f, 0.0f, -1.0f));

	Window window(resX, resY, "Vulkan");
	VulcanInstance vulcanInstance(window.getWindow(), resX, resY);
	RayTracer rayTracer(vulcanInstance.m_PhysicalDevice, vulcanInstance.m_Device, vulcanInstance.getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT), vulcanInstance.m_CommandPool, vulcanInstance.m_GraphicQueue, vulcanInstance.m_RenderPass, 1024, 1024);
	SceneObjectFactory sceneObjectFactory(vulcanInstance.m_PhysicalDevice, vulcanInstance.m_Device, vulcanInstance.m_CommandPool, vulcanInstance.m_GraphicQueue);

	TextureManager textureManager(vulcanInstance.m_PhysicalDevice, vulcanInstance.m_Device, vulcanInstance.m_CommandPool, vulcanInstance.m_GraphicQueue);

	
	auto frontTex = textureManager.loadTexture("C:/Phoenix/Models/front.jpg");
	auto rightTex = textureManager.loadTexture("C:/Phoenix/Models/right.jpg");
	auto leftTex = textureManager.loadTexture("C:/Phoenix/Models/left.jpg");
	auto backTex = textureManager.loadTexture("C:/Phoenix/Models/back.jpg");
	auto topTex = textureManager.loadTexture("C:/Phoenix/Models/top.jpg");
	auto bottomTex = textureManager.loadTexture("C:/Phoenix/Models/bottom.jpg");


	TextureHelper textureHelper;
	textureHelper.add("front", frontTex->imageView);
	textureHelper.add("back", backTex->imageView);
	textureHelper.add("left", leftTex->imageView);
	textureHelper.add("right", rightTex->imageView);
	textureHelper.add("top", topTex->imageView);
	textureHelper.add("bottom", bottomTex->imageView);

	/*
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
	
	*/
	
	
	//{ frontTex->imageView, rightTex->imageView, leftTex->imageView, backTex->imageView, topTex->imageView, bottomTex->imageView })
	vector<Triangle> triangles;

	float scale = 5000.0f;

	MaterialHelper materialHelper;

	Material material;
	material.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	material.reflFactor = 0.0f;
	material.texId = textureHelper.find("front");
	materialHelper.add("front", material);

	material.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	material.reflFactor = 0.0f;
	material.texId = textureHelper.find("back");
	materialHelper.add("back", material);

	material.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	material.reflFactor = 0.0f;
	material.texId = textureHelper.find("bottom");
	materialHelper.add("bottom", material);

	material.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	material.reflFactor = 0.0f;
	material.texId = textureHelper.find("top");
	materialHelper.add("top", material);

	material.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	material.reflFactor = 0.0f;
	material.texId = textureHelper.find("left");
	materialHelper.add("left", material);

	material.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	material.reflFactor = 0.0f;
	material.texId = textureHelper.find("right");
	materialHelper.add("right", material);

	//front side
	Triangle face;
	face.v0 = glm::vec3(-0.5, -0.5f, -0.5f) *scale;	 face.t0 = glm::vec2(0.0f, 1.0f);
	face.v1 = glm::vec3(0.5, -0.5f, -0.5f)  *scale;	 face.t1 = glm::vec2(1.0f, 1.0f);
	face.v2 = glm::vec3(0.5, 0.5f, -0.5f)  *scale;	 face.t2 = glm::vec2(1.0f, 0.0f);
	face.materialId = materialHelper.find("front");
	triangles.push_back(face);

	face.v0 = glm::vec3(-0.5, -0.5f, -0.5f) *scale;		face.t0 = glm::vec2(0.0f, 1.0f);
	face.v1 = glm::vec3(0.5, 0.5f, -0.5f) * scale;		 face.t1 = glm::vec2(1.0f, 0.0f);
	face.v2 = glm::vec3(-0.5, 0.5f, -0.5f) * scale;		 face.t2 = glm::vec2(0.0f, 0.0f);
	face.materialId = materialHelper.find("front");
	triangles.push_back(face);


	
	//back side
	face.v1 = glm::vec3(-0.5, -0.5f, 0.5f) * scale;	 face.t1 = glm::vec2(1.0f, 1.0f);
	face.v0 = glm::vec3(0.5, -0.5f, 0.5f) * scale;	 face.t0 = glm::vec2(0.0f, 1.0f);
	face.v2 = glm::vec3(0.5, 0.5f, 0.5f) * scale;	 face.t2 = glm::vec2(0.0f, 0.0f);
	face.materialId = materialHelper.find("back");
	triangles.push_back(face);

	face.v0 = glm::vec3(-0.5, -0.5f, 0.5f) * scale;		face.t0 = glm::vec2(1.0f, 1.0f);
	face.v2 = glm::vec3(0.5, 0.5f, 0.5f) * scale;		 face.t2 = glm::vec2(0.0f, 0.0f);
	face.v1 = glm::vec3(-0.5, 0.5f, 0.5f) * scale;		 face.t1 = glm::vec2(1.0f, 0.0f);
	face.materialId = materialHelper.find("back");
	triangles.push_back(face);


	//floor
	face.v0 = glm::vec3(-0.5, -0.5f, -0.5f) * scale;	 face.t0 = glm::vec2(0.0f, 0.0f);
	face.v1 = glm::vec3(-0.5, -0.5f, 0.5f) * scale;	 face.t1 = glm::vec2(0.0f, 1.0f);
	face.v2 = glm::vec3(0.5, -0.5f, 0.5f) * scale;	 face.t2 = glm::vec2(1.0f, 1.0f);
	face.materialId = materialHelper.find("bottom");
	triangles.push_back(face);

	face.v0 = glm::vec3(-0.5, -0.5f, -0.5f) * scale;	 face.t0 = glm::vec2(0.0f, 0.0f);
	face.v2 = glm::vec3(0.5, -0.5f, -0.5f) * scale;	 face.t2 = glm::vec2(1.0f, 0.0f);
	face.v1 = glm::vec3(0.5, -0.5f, 0.5f) * scale;	 face.t1 = glm::vec2(1.0f, 1.0f);
	face.materialId = materialHelper.find("bottom");
	triangles.push_back(face);

	

	//ceiling
	face.v0 = glm::vec3(-0.5, 0.5f, -0.5f) * scale;	 face.t0 = glm::vec2(0.0f, 1.0f);
	face.v2 = glm::vec3(-0.5, 0.5f, 0.5f) * scale;	 face.t2 = glm::vec2(0.0f, 0.0f);
	face.v1 = glm::vec3(0.5, 0.5f, 0.5f) * scale;	 face.t1 = glm::vec2(1.0f, 0.0f);
	face.materialId = materialHelper.find("top");
	triangles.push_back(face);

	face.v0 = glm::vec3(-0.5, 0.5f, -0.5f) * scale;	 face.t0 = glm::vec2(0.0f, 1.0f);
	face.v1 = glm::vec3(0.5, 0.5f, -0.5f) * scale;	 face.t1 = glm::vec2(1.0f, 1.0f);
	face.v2 = glm::vec3(0.5, 0.5f, 0.5f) * scale;	 face.t2 = glm::vec2(1.0f, 0.0f);
	face.materialId = materialHelper.find("top");
	triangles.push_back(face);

	

	//left side
	face.v0 = glm::vec3(-0.5, -0.5f, -0.5f) * scale; face.t0 = glm::vec2(1.0f, 1.0f);
	face.v1 = glm::vec3(-0.5, 0.5f, -0.5f) * scale;	 face.t1 = glm::vec2(1.0f, 0.0f);
	face.v2 = glm::vec3(-0.5, 0.5f, 0.5f) * scale;	 face.t2 = glm::vec2(0.0f, 0.0f);
	face.materialId = materialHelper.find("left");
	triangles.push_back(face);

	face.v0 = glm::vec3(-0.5, -0.5f, -0.5f) * scale;	face.t0 = glm::vec2(1.0f, 1.0f);
	face.v1 = glm::vec3(-0.5, 0.5f, 0.5f) * scale;		 face.t1 = glm::vec2(0.0f, 0.0f);
	face.v2 = glm::vec3(-0.5, -0.5f, 0.5f) * scale;		 face.t2 = glm::vec2(0.0f, 1.0f);
	face.materialId = materialHelper.find("left");
	triangles.push_back(face);



	//right side
	face.v0 = glm::vec3(0.5, -0.5f, -0.5f) * scale;	 face.t0 = glm::vec2(0.0f, 1.0f);
	face.v2 = glm::vec3(0.5, 0.5f, -0.5f) * scale;	 face.t2 = glm::vec2(0.0f, 0.0f);
	face.v1 = glm::vec3(0.5, 0.5f, 0.5f) * scale;	 face.t1 = glm::vec2(1.0f, 0.0f);
	face.materialId = materialHelper.find("right");
	triangles.push_back(face);
	
	face.v0 = glm::vec3(0.5, -0.5f, -0.5f) * scale;		face.t0 = glm::vec2(0.0f, 1.0f);
	face.v2 = glm::vec3(0.5, 0.5f, 0.5f) * scale;		 face.t2 = glm::vec2(1.0f, 0.0f);
	face.v1 = glm::vec3(0.5, -0.5f, 0.5f) * scale;		 face.t1 = glm::vec2(1.0f, 1.0f);
	face.materialId = materialHelper.find("right");
	triangles.push_back(face);


	//mirror deck
	material.color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	material.reflFactor = 0.9f;
	material.texId = textureHelper.find("mirror");
	materialHelper.add("mirror", material);

	face.v0 = glm::vec3(-550, 0, -550);	 face.t0 = glm::vec2(0.0f, 0.0f);
	face.v1 = glm::vec3(-550, 0, 550);	 face.t1 = glm::vec2(0.0f, 1.0f);
	face.v2 = glm::vec3(550, 0, 550);	 face.t2 = glm::vec2(1.0f, 1.0f);
	face.materialId = materialHelper.find("mirror");
	triangles.push_back(face);

	face.v0 = glm::vec3(-550, 0, -550);	 face.t0 = glm::vec2(0.0f, 0.0f);
	face.v2 = glm::vec3(550, 0, -550);	 face.t2 = glm::vec2(1.0f, 0.0f);
	face.v1 = glm::vec3(550, 0, 550);	 face.t1 = glm::vec2(1.0f, 1.0f);
	face.materialId = materialHelper.find("mirror");
	triangles.push_back(face);


	vector<Sphere> spheres;

	for (int i = 0; i < 40; ++i)
	{
		char buff[10];
		string sphereName = "sphere_" + string(itoa(i, buff, 10));

		material.color = glm::vec4((rand()%1000) /1000.0f, (rand() % 1000) / 1000.0f, (rand() % 1000) / 1000.0f, 1.0f);
		material.reflFactor = (rand() % 1000) / 1000.0f;
		material.texId = -1;
		materialHelper.add(sphereName, material);
		

		Sphere sphere;
		
		sphere.radius = 50.0f + (rand() % 100) / 100.0f * 50.0f;

		int iterCnt = 0;

		bool noCollision;
		do
		{
			noCollision = true;

			sphere.pos.x = (-0.5f + (rand() % 100) / 100.0f) * 1000.0f;
			sphere.pos.z = (-0.5f + (rand() % 100) / 100.0f) * 1000.0f;
			sphere.pos.y = sphere.radius;

			for (auto& s : spheres)
			{
				if (glm::distance(s.pos, sphere.pos) < s.radius + sphere.radius)
				{
					noCollision = false;
					break;
				}
			}

			++iterCnt;
			if (iterCnt > 50)
			{
				sphere.radius -= sphere.radius * 0.1;
			}
		} while (!noCollision);
	

		sphere.materialId = materialHelper.find(sphereName);

		spheres.push_back(sphere);
	}


	


	rayTracer.setTextures(textureHelper.getDataArray());
	rayTracer.setTriangles(triangles);
	rayTracer.setSpheres(spheres);
	rayTracer.setMaterials(materialHelper.getDataArray());

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

