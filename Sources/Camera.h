#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>


class Camera
{
private:
	glm::mat4 m_Matrix;

public:
	Camera();
	void rotate(float rightAxis, float upAxis);
	void move(float dir, float right, float up);
	void setPos(glm::vec3 const& pos);
	void setDir(glm::vec3 const& dir); 
	glm::mat4& getMatrix();
	glm::mat4 getInvMatrix();
};
