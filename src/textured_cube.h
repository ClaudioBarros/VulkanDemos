#ifndef TEXTURED_CUBE_H
#define TEXTURED_CUBE_H

#include <vector>
#include <platform.h> 
#include <vulkan_manager.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>

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

	Demo(){}
	~Demo(){}

	void startUp();
	void shutDown();

	void prepare();
	void initCubeDataBuffers();
	void initDescriptorLayout();
	void initRenderPass();
	void initPipeline();
};

struct Texture
{
	uint8 *pixels;	
	uint32 width;
	uint32 height;
	std::string filepath;

	void load(std::string filepath);
	void free(); 
};

struct VS_UBO 
{
	alignas(16) glm::mat4 mvp;
	alignas(16) glm::vec4 pos;
	alignas(16) glm::vec2 uv;
};

void loadShaderModule(std::string &filename, std::vector<char> &buffer);

#endif 