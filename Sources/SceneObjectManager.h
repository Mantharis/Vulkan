#pragma once

#include "SceneObject.h"

#include <memory>
#include <algorithm>

class SceneObjectManager
{
public:
	void insert(unique_ptr<SceneObject>&& sceneObject);
	unique_ptr<SceneObject> remove(SceneObject* sceneObject);
	template<typename Func> void enumerate(Func func)
	{
		for_each(m_SceneObjects.begin(), m_SceneObjects.end(), [func](unique_ptr<SceneObject>& obj)
			{
				func(obj.get());
			});
	}

	void clear();
private:
	vector<unique_ptr<SceneObject>> m_SceneObjects;
};
