#pragma once

struct FPSInput 
{
	float deadzone;
	float minRadius;
	float maxRadius;

	float mouseX;
	float mouseY;
	float mouseLastX;
	float mouseLastY;
	
	float xOffset;
	float yOffset;
	
	bool dirUp;
	bool dirDown;
	bool dirLeft;
	bool dirRight;	
};
