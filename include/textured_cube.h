#ifndef TEXTURED_CUBE_H
#define TEXTURED_CUBE_H

#include <vector>
#include <platform.h> 
#include <vulkan_manager.h>
#include <camera.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>

struct Texture
{
	uint8 *pixels;	
	uint32 width;
	uint32 height;
	uint32 channels;
	std::string filepath;

	void load(std::string filepath);
	void free(); 
};

struct VS_UBO 
{
	alignas(16) glm::mat4 mvp;
	alignas(16) glm::vec4 pos[12 * 3];
	alignas(16) glm::vec4 attr[12 * 3];
};

void loadShaderModule(std::string &filename, std::vector<char> &buffer);

struct Demo
{
	Win32Window window;
	VulkanManager vulkanManager;
	Camera camera;

	glm::mat4 modelMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 projMatrix;

	std::vector<float> vertexData;
	std::vector<float> uvData;

	std::vector<Texture> textures;
	std::vector<VulkanTexture> vulkanTextures;
	VulkanTexture stagingTexture;
	
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	
	uint32 currBufferIndex = 0;
	int frameIndex = 0;	

	float lastFrameTime; //seconds

	uint32 width;
	uint32 height;
	
	float mouseLastX;
	float mouseLastY;
	float mouseSensitivity;
	bool  firstMouseInput = true;

	bool isInitialized;
	bool isMinimized;	
	bool isPaused;
	bool isPrepared;
	
	float movementSpeed; //units per second

	Demo(){}
	~Demo(){}

	void startUp();
	void shutDown();
	
	void processKeyboardInput(uint64 *pressedKey);
	void centerMouseCursor();
	void confineMouseCursorToWindow();
	void processMouseInput(float xPos, float yPos);

	void prepare();
	void initStagingTexture();
	void initTextures();
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
	void updateAndRender();	
};


#endif 