#pragma once

#include "SceneObject.h"

#include <memory>
#include <algorithm>

class SceneObjectManager
{
public:
	void insert(unique_ptr<SceneObject>&& sceneObject)
	{
		m_SceneObjects.push_back(move(sceneObject));
	}

	unique_ptr<SceneObject> remove(SceneObject* sceneObject)
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

	template<typename Func> void enumerate(Func func)
	{
		for_each(m_SceneObjects.begin(), m_SceneObjects.end(), [func](unique_ptr<SceneObject>& obj)
			{
				func(obj.get());
			});
	}

	void clear()
	{
		m_SceneObjects.clear();
	}
private:

	vector<unique_ptr<SceneObject>> m_SceneObjects;
};
