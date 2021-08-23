#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Camera
{
	glm::vec3 pos;
	glm::vec3 fwd;
	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 worldUp;

	float yaw;
	float pitch;

	void init(glm::vec3 position, float yawAngle,  float pitchAngle) 
	{
		pos = position;
		worldUp = glm::vec3(0, 1, 0);
		up = glm::vec3(0, 1, 0);
		yaw = yawAngle;
		pitch = pitchAngle;
		updateVectors();
	}
	
	glm::mat4 getViewMatrix()
	{
		return glm::lookAt(pos, (pos + fwd), up);
	}
	
	void moveForward(float speed, float deltaTime)
	{
		pos += speed * deltaTime * fwd; 
	}
	
	void moveBackwards(float speed, float deltaTime)
	{
		pos -= speed * deltaTime * fwd; 
	}

	void moveLeft(float speed, float deltaTime)
	{
		pos -= speed * deltaTime * right; 
	}

	void moveRight(float speed, float deltaTime)
	{
		pos += speed * deltaTime * right; 
	}
	
	void rotate(float xOffset, float yOffset, float sensitivity)
	{
		xOffset = xOffset * sensitivity;
		yOffset = yOffset * sensitivity;
		
		//constrain yaw to [0, 360] degree range to avoid 
		//floating point imprecision at high values 
		yaw = glm::mod(yaw + xOffset, 360.0f); 		
		pitch += yOffset;
		
		if(pitch >  89.0f) pitch =  89.0f;
		if(pitch < -89.0f) pitch = -89.0f;
		
		updateVectors();
	}

	void updateVectors()
	{
		glm::vec3 forward;
		forward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		forward.y = sin(glm::radians(pitch));
		forward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		fwd = glm::normalize(forward);
		
		right = glm::normalize(glm::cross(fwd, worldUp));
		up = glm::normalize(glm::cross(right, fwd));
	}	
};

#endif