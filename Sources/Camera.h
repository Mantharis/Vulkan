#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <glm/gtc/matrix_transform.hpp>

class Camera
{
private:
	glm::mat4 m_Matrix;

public:
	Camera() :m_Matrix(1.0f)
	{
	}

	void rotate(float rightAxis, float upAxis)
	{
		auto pos = m_Matrix[3];

		glm::quat upRot = glm::angleAxis(glm::radians(upAxis), glm::vec3(0.f, 1.f, 0.f));
		glm::quat rightRot = glm::angleAxis(glm::radians(rightAxis), glm::vec3(m_Matrix[0].x, m_Matrix[0].y, m_Matrix[0].z));

		auto finalRot = rightRot * upRot;
		auto newDir = finalRot * glm::vec3(-m_Matrix[2].x, -m_Matrix[2].y, -m_Matrix[2].z);

		setDir(newDir);
	}

	void move(float dir, float right, float up)
	{
		glm::vec4 offset = (-dir * m_Matrix[2]) + (up * m_Matrix[1]) + (right * m_Matrix[0]);
		m_Matrix[3] += offset;
	}

	void setPos(glm::vec3 const& pos)
	{
		m_Matrix[3].x = pos.x;
		m_Matrix[3].y = pos.y;
		m_Matrix[3].z = pos.z;
		m_Matrix[3].w = 1.0f;
	}

	void setDir(glm::vec3 const& dir)
	{
		auto nDir = glm::normalize(dir * -1.0f);
		m_Matrix[2].x = nDir.x;
		m_Matrix[2].y = nDir.y;
		m_Matrix[2].z = nDir.z;
		m_Matrix[2].w = 0;

		auto nRight = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), nDir));
		m_Matrix[0].x = nRight.x;
		m_Matrix[0].y = nRight.y;
		m_Matrix[0].z = nRight.z;
		m_Matrix[0].w = 0.0f;

		auto nUp = glm::normalize(glm::cross(nDir, nRight));
		m_Matrix[1].x = nUp.x;
		m_Matrix[1].y = nUp.y;
		m_Matrix[1].z = nUp.z;
		m_Matrix[1].w = 0.0f;
	}

	glm::mat4& getMatrix()
	{
		return m_Matrix;
	}

	glm::mat4 getInvMatrix()
	{
		return glm::inverse(m_Matrix);
	}
};
