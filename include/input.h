#ifndef INPUT_H
#define INPUT_H

struct Input 
{
	float mouseX;
	float mouseY;
	float mouseLastX;
	float mouseLastY;

	bool dirUp;
	bool dirDown;
	bool dirLeft;
	bool dirRight;	
	
	void resetKeys()
	{
		dirUp = false;
		dirDown = false;
		dirLeft = false;
		dirRight = false;
	}
	
	void resetMouseCurrPos()
	{
		mouseX = 0.0f;
		mouseY = 0.0f;
	}
	
	void resetMouseLastPos()
	{
		mouseLastX = 0.0f;
		mouseLastY = 0.0f;
	}
};

#endif