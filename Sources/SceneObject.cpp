
#include "SceneObject.h"

using namespace std;


	void SceneObject::addComponent(unique_ptr<IComponent> comp)
	{
		m_Components.push_back(move(comp));
	}

	glm::mat4& SceneObject::getMatrix()
	{
		return m_Matrix;
	}
