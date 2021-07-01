#ifndef TEXTURED_CUBE_H
#define TEXTURED_CUBE_H

#include <vector>

struct Demo
{
	VulkanManager vulkanManager;

	const std::vector<float> vertexData;
	const std::vector<float> uvData;	
};

#endif 