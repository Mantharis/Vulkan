#pragma once

#include "SceneObjectManager.h"



	void SceneObjectManager::insert(unique_ptr<SceneObject>&& sceneObject)
	{
		m_SceneObjects.push_back(move(sceneObject));
	}

	unique_ptr<SceneObject> SceneObjectManager::remove(SceneObject* sceneObject)
	{
		auto it = find_if(m_SceneObjects.begin(), m_SceneObjects.end(), [sceneObject](unique_ptr<SceneObject>& obj)
			{
				return obj.get() == sceneObject;
			});

		if (it == m_SceneObjects.end()) return unique_ptr<SceneObject>();
		else
		{
			swap(*it, m_SceneObjects.back());

			auto scenObjToRemove = move(m_SceneObjects.back());
			m_SceneObjects.pop_back();

			return scenObjToRemove;
		}
	}

	void SceneObjectManager::clear()
	{
		m_SceneObjects.clear();
	}
