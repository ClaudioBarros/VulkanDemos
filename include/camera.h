#pragma once

#include "vectors.h"
#include "matrix.h"

struct Camera
{
	vec3 pos;
	vec3 fwd;
	vec3 right;
	vec3 up;
	vec3 worldUp = vec3(0.0f, 1.0f, 0.0f);

	float yaw;
	float pitch;

	void init(vec3 position, float yawAngle,  float pitchAngle) 
	{
		pos = position;
		worldUp = vec3i(0, 1, 0);
		up = vec3i(0, 1, 0);
		yaw = yawAngle;
		pitch = pitchAngle;
		updateVectors();
	}

	mat4 getViewMatrix()
	{
		return lookAt(pos, (pos + fwd), up);
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
		yaw = fmod(yaw + xOffset, 360.0f); 		
		pitch += yOffset;
		
		if(pitch >  89.0f) pitch =  89.0f;
		if(pitch < -89.0f) pitch = -89.0f;
		
		updateVectors();
	}

	void updateVectors()
	{
		vec3 forward;
		forward = vec3(cos(DEG2RAD(yaw)) * cos(DEG2RAD(pitch)),  //x
		               sin(DEG2RAD(pitch)),                       //y
		               sin(DEG2RAD(yaw)) * cos(DEG2RAD(pitch))); //z
		fwd = normalize(forward);

		right = normalize(cross(fwd, worldUp));
		up = normalize(cross(right, fwd));
	}	
};
