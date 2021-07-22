#ifndef TEXTURED_CUBE_H
#define TEXTURED_CUBE_H

#include <vector>
#include <platform.h> 
#include <vulkan_manager.h>
#include <camera.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>

#define MAX_FRAMES 3

struct Demo
{
	Win32Window window;
	VulkanManager vulkanManager;
	Camera camera;

	VkSemaphore imageAcquiredSemaphores[MAX_FRAMES];
	VkSemaphore drawCompleteSemaphores[MAX_FRAMES];
	VkSemaphore imageOwnershipSemaphores[MAX_FRAMES];
	VkFence fences[MAX_FRAMES];
	int frameIndex;	

	std::vector<float> vertexData;
	std::vector<float> uvData;

	std::vector<Texture> textures;
	std::vector<VulkanTexture> vulkanTextures;
	VulkanTexture stagingTexture;
	
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	
	uint32 currBufferIndex;

	bool isMinimized;	
	bool prepared;
	

	Demo(){}
	~Demo(){}

	void startUp();
	void shutDown();
	
	void prepare();
	void initCubeDataBuffers();
	void initDescriptorLayout();
	void initRenderPass();
	void initPipeline();
	void setupImageOwnership(int i);
	void initDescriptorPool();
	void initDescriptorSet();
	void initFramebuffers();
	void recordDrawCommands(VkCommandBuffer cmdBuffer);
	void flushInitCmd();
	void resize();
	void updateDataBuffer();
	void draw();	
	void run();
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