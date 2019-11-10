#pragma once

#include "Model.h"
#include "ModelLoader.h"
#include "VertexFormat.h"

#include <memory>
#include <assert.h>
#include <unordered_map>
#include <vector>

using ModelDataSharedPtr = shared_ptr<const ModelData>;

class ModelManager
{
public:
	ModelManager(VkPhysicalDevice physicalDevice, VkDevice device, MaterialManager& materialManager);
	~ModelManager();
	ModelDataSharedPtr loadModel(string const& path);

private:
	MaterialManager& m_MaterialManager;

	unordered_map<string, weak_ptr <const ModelData> > m_Models;
	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_Device;
};