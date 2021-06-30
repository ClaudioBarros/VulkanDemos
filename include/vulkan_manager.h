#ifndef VULKAN_MANAGER_H
#define VULKAN_MANAGER_H

#define VK_USE_PLATFORM_WIN32_KHR 

#include "vulkan/vulkan.h"
#include <vector>
#include <string>
#include "platform.h"
#include "typedefs_and_macros.h"


struct VulkanManager
{
	Win32Window *window;
	VulkanConfig config;

	VkInstance instance;

	VkSurfaceKHR surface;

	PhysicalDevice physicalDevice;

	LogicalDevice logicalDevice;

	Swapchain swapchain;
	
	VkRenderPass renderPass;

	VkCommandPool cmdPool;
	VkCommandBuffer cmdBuffer; //used for initialization

	Depth depth;

	std::vector<Texture> textures;
	Texture stagingTexture;

	VulkanManager(){} //do nothing
	~VulkanManager(){} //do nothing

	void startUp(Win32Window *window, VulkanConfig config);
	void shutDown();

	void initInstance();
	void initSurface(Win32Window *window);
	void initPhysicalDevice(VkPhysicalDeviceFeatures featuresToEnable);
	void initRenderPass();
	void initCmdPool();
	void initDepthImage();
	void initDepthImage(VkFormat depthFormat, uint32 width, uint32 height);
	
	void initBuffer(VkDeviceSize size, 
					VkBufferUsageFlags usageFlags, 
					VkMemoryPropertyFlags propertyFlags,
					VkBuffer &buffer, 
					VkDeviceMemory &bufferMemory);
					
	void initImage(uint32 width, uint32 height,
				   VkFormat format, VkImageTiling tiling, 
				   VkImageUsageFlags usage, VkMemoryPropertyFlags propertyFlags, 
				   VkImage &image, VkDeviceMemory &imageMemory);

	void setImageLayout(VkImage image, 
					    VkImageAspectFlags aspectMask,
						VkImageLayout oldLayout, 
						VkImageLayout newLayout, 
						VkAccessFlagBits srcAccessMask, 
						VkPipelineStageFlags srcStages, 
						VkPipelineStageFlags destStages);

	void initTextures();
};

struct VulkanConfig
{
	std::string appName;
	bool enableValidationLayers;
	std::string errorLogFilePath;

	//physical device
	VkPhysicalDeviceFeatures physDeviceFeaturesToEnable;

	//swapchain
	VkSurfaceFormatKHR preferredSurfaceFormat;
	VkPresentModeKHR preferredPresentMode;

	//depth buffer
	VkFormat preferredDepthFormat;

	//textures
	VkFormat texFormat;
	uint32 texCount;
	std::vector<std::string> texFiles;
	
	std::vector<const char*> validationLayers;
	std::vector<const char*> deviceExtensions;
};

struct Depth
{
	VkFormat format;
	VkImage image;
	VkMemoryAllocateInfo memAlloc;
	VkDeviceMemory mem;
	VkImageView view;
};

struct Texture
{
    VkSampler sampler;

    VkImage image;
    VkBuffer buffer;
    VkImageLayout imageLayout;

    VkMemoryAllocateInfo memAlloc;
    VkDeviceMemory mem;
    VkImageView view;
    int32 width, height;
};

struct PhysicalDevice
{
	VkPhysicalDevice device;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures supportedFeatures;
	VkPhysicalDeviceFeatures enabledFeatures;
	VkPhysicalDeviceMemoryProperties memProperties;

	std::vector<VkQueueFamilyProperties> queueFamilyProperties;

	uint32 graphicsQueueFamilyIndex;
	uint32 presentQueueFamilyIndex;

	void init(VkPhysicalDevice physicalDevice, 
	          VkSurfaceKHR surface,
			  VkPhysicalDeviceFeatures featuresToEnable);
	void destroy();
};

struct LogicalDevice
{
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	void init(PhysicalDevice &physicalDevice,
	          VulkanConfig config);

	void destroy();
};

struct SwapchainImageResources 
{
    VkImage image;
    VkCommandBuffer cmd;
    VkCommandBuffer graphicsToPresentCmd;
    VkImageView view;
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformMemory;
    void *uniformMemoryPtr;
    VkFramebuffer framebuffer;
    VkDescriptorSet descriptorSet;
};

struct Swapchain
{
	VkSwapchainKHR swapchain;
	VkSurfaceCapabilitiesKHR surfaceCapabilities;	
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;
	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;
	VkExtent2D imageExtent;

	std::vector<SwapchainImageResources> imageResources;

	private:

 	void querySupportInfo(VkPhysicalDevice physicalDevice,  
	  					  VkSurfaceKHR surface);

	void chooseSettings(VkSurfaceFormatKHR preferredFormat,
						VkPresentModeKHR preferredPresentMode,
						uint32 width, uint32 height);

	void createSwapchainAndImageResources(VkSurfaceKHR surface, 
						 VkDevice logicalDevice);

	public:

 	void init(VkPhysicalDevice physicalDevice,  
	 		  VkDevice logicalDevice,
			  VkSurfaceKHR surface,
			  VkSurfaceFormatKHR preferredFormat,
			  VkPresentModeKHR preferredPresentMode,
			  uint32 surfaceWidth, uint32 surfaceHeight);

	void destroy();
};

uint32 findMemoryTypeFromProperties(const VkPhysicalDeviceMemoryProperties *pMemoryProperties,
                                    uint32 memoryTypeBitsRequired, 
                                    VkMemoryPropertyFlags requiredProperties);

#endif