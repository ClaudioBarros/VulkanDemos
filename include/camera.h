#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

struct Camera
{
	glm::vec3 pos;
	glm::vec3 dir;
	glm::vec3 right;
	glm::vec3 up;
	
	glm::mat4 viewMatrix;
	glm::mat4 projMatrix;

	struct Frustum
	{
		float near_;
		float far_;
		float right;
		float left;
		float top;
		float bottom;
	} frustum;

	void init(glm::vec3 position, glm::vec3 target,
			  float near_,  float far_,
			  float right, float left,
			  float top,   float bottom)
	{
		pos = position;
		lookAt(target);

		frustum.near_ = near_;
		frustum.far_ = far_;
		frustum.right = right;
		frustum.left = left;
		frustum.top = top;
		frustum.bottom = bottom;
	}

	void lookAt(glm::vec3 target)
	{
		dir = glm::normalize(pos - target);
		
		glm::vec3 temp(0.0f, 1.0f, 0.0f);
		right = glm::normalize(glm::cross(temp, dir));
		
		up = glm::cross(dir, right);

		glm::mat4 rotation(1.0f);	
		glm::mat4 translation(1.0f);
		
		rotation[0][0] = right.x;
		rotation[0][1] = right.y;
		rotation[0][2] = right.z;
		rotation[1][0] = up.x;
		rotation[1][1] = up.y;
		rotation[1][2] = up.z;
		rotation[2][0] = dir.x;
		rotation[2][1] = dir.y;
		rotation[2][2] = dir.z;

		translation[0][3] = -pos.x;
		translation[1][3] = -pos.y;
		translation[2][3] = -pos.z;
		
		viewMatrix = rotation * translation;
	}

	void calcProjMatrix()
	{
		float m00 = (2.0f * frustum.near_)/(frustum.right - frustum.left);
		float m02 = (frustum.right + frustum.left)/(frustum.right - frustum.left);
		
		float m11 = (2.0f * frustum.near_)/(frustum.top - frustum.bottom);
		float m12 = (frustum.top + frustum.bottom)/(frustum.top - frustum.bottom);
		
		float m22 = -(frustum.far_ + frustum.near_)/(frustum.top - frustum.near_);
		float m23 = (-2.0f * frustum.far_ * frustum.near_)/(frustum.far_ - frustum.near_);

		projMatrix = glm::mat4(m00,  0.0f,  m02,   0.0f,
		                       0.0f, m11,   m12,   0.0f,
							   0.0f, 0.0f,  m22,   m23,
							   0.0f, 0.0f, -1.0f,  0.0f);
	}
};

#endif