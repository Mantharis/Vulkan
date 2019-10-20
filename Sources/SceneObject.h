#pragma once

#include <algorithm>
#include <memory>
#include <glm/mat4x4.hpp>

using namespace std;

enum class ComponentType
{
	VISUAL_COMPONENT
};

class IComponent
{
public:
	virtual ~IComponent() {};
	virtual ComponentType GetComponentType() = 0;
};


class SceneObject
{
public:
	void addComponent(unique_ptr<IComponent> comp)
	{
		m_Components.push_back(move(comp));
	}

	glm::mat4& getMatrix()
	{
		return m_Matrix;
	}

	template<typename COMP> COMP* findComponent()
	{
		ComponentType type = COMP::GetStaticComponentType();

		auto it = find_if(m_Components.begin(), m_Components.end(), [type](unique_ptr<IComponent>& comp)
			{
				return type == comp->GetComponentType();
			});

		if (it != m_Components.end())
		{
			return dynamic_cast<COMP*>(it->get());
		}
		else
		{
			return nullptr;
		}
	}

private:
	vector<unique_ptr<IComponent>> m_Components;
	glm::mat4 m_Matrix;
};
