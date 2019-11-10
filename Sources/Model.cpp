#pragma once

#include "Model.h"


using namespace std;


	MeshData::~MeshData()
	{
		destroy();
	}

	MeshData& MeshData::operator=(MeshData&& obj)
	{
		destroy();
		*this = obj;
		obj.m_Device = VK_NULL_HANDLE;

		return *this;
	}

	MeshData::MeshData(MeshData&& obj)
	{
		*this = move(obj);
	}

	void MeshData::destroy()
	{
		if (m_Device == VK_NULL_HANDLE) return;

		vkDestroyBuffer(m_Device, vertexBufffer, nullptr);
		vkFreeMemory(m_Device, vertexBufferMemory, nullptr);

		vkDestroyBuffer(m_Device, indexBuffer, nullptr);
		vkFreeMemory(m_Device, indexBufferMemory, nullptr);

		m_Device = VK_NULL_HANDLE;
	}
