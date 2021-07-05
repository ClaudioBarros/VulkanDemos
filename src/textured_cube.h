#ifndef TEXTURED_CUBE_H
#define TEXTURED_CUBE_H

#include <vector>
#include <platform.h> 
#include <vulkan_manager.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct Demo
{
	Win32Window window;
	VulkanManager vulkanManager;

	std::vector<float> vertexData;
	std::vector<float> uvData;

	VkCommandBuffer cmdBuffer;

	std::vector<Texture> textures;
	std::vector<VulkanTexture> vulkanTextures;
	VulkanTexture stagingTexture;

	Demo() {}
	~Demo() {}

	void startUp();
	void shutDown();

	void prepare();
};

struct Texture
{
	uint8 *pixels;	
	uint32 width;
	uint32 height;
	std::string filepath;

	void load(std::string filepath);
	void free(); 
}


#endif 