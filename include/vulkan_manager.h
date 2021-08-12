#ifndef VULKAN_MANAGER_H
#define VULKAN_MANAGER_H

#define VK_USE_PLATFORM_WIN32_KHR 

#include "vulkan/vulkan.h"
#include <vector>
#include <string>
#include "platform.h"
#include "typedefs_and_macros.h"

#define MAX_FRAMES 3

// STRUCTS AND HELPER FUNCTIONS 
//==================== Vulkan Config ======================
struct VulkanConfig
{
	std::string appName;
	bool enableValidationLayers;
	std::string errorLogFilePath;

	//debug callback ptr
    PFN_vkDebugUtilsMessengerCallbackEXT ptrDebugMessenger;

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
	std::vector<const char*> instanceExtensions;
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

// ========================= Vulkan Texture ====================
struct VulkanTexture
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


// ========================= Physical Device =========================
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
	
	bool separatePresentQueue;

	void init(VkPhysicalDevice physicalDevice, 
	          VkSurfaceKHR surface,
			  VkPhysicalDeviceFeatures featuresToEnable);
	void destroy();
};

uint32 findMemoryTypeFromProperties(const VkPhysicalDeviceMemoryProperties *pMemoryProperties,
                                    uint32 memoryTypeBitsRequired, 
                                    VkMemoryPropertyFlags requiredProperties);
// ======================== Logical Device ==========================
struct LogicalDevice
{
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	void init(PhysicalDevice &physicalDevice,
	          VulkanConfig config);

	void destroy();
};

// ========================= Swapchain ============================
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

	uint32 imageCount;
	std::vector<SwapchainImageResources> imageResources;


	void querySupportInfo(VkPhysicalDevice physicalDevice,  
						  VkSurfaceKHR surface);

	void chooseSettings(VkSurfaceFormatKHR preferredFormat,
						VkPresentModeKHR preferredPresentMode,
						uint32 *width, uint32 *height);


	void queryInfoAndChooseSettings(VkPhysicalDevice physicalDevice,  
									VkDevice logicalDevice,
									VkSurfaceKHR surface,
									VkSurfaceFormatKHR preferredFormat,
									VkPresentModeKHR preferredPresentMode,
									uint32 *demoWidth, uint32 *demoHeight);
		
	void createSwapchainAndImageResources(VkSurfaceKHR surface, VkDevice logicalDevice);

	void destroy(VkDevice &device, VkCommandPool &cmdPool);
};

//------------------------
void allocPrimaryCmdBuffer(VkDevice device, VkCommandPool &cmdPool, VkCommandBuffer &cmdBuffer);

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
                                      const VkAllocationCallbacks* pAllocator, 
                                      VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance, 
                                   VkDebugUtilsMessengerEXT debugMessenger, 
                                   const VkAllocationCallbacks* pAllocator) ;
//==================================================================

struct VulkanManager
{
	Win32Window *window;
	VulkanConfig config;
	
	VkDebugUtilsMessengerEXT debugMessenger;

	VkInstance instance;

	VkSurfaceKHR surface;

	PhysicalDevice physicalDevice;

	LogicalDevice logicalDevice;

	Swapchain swapchain;
	
	bool *isMinimized;
	
	VkSemaphore imageAcquiredSemaphores[MAX_FRAMES];
	VkSemaphore drawCompleteSemaphores[MAX_FRAMES];
	VkSemaphore imageOwnershipSemaphores[MAX_FRAMES];
	VkFence fences[MAX_FRAMES];

	VkRenderPass renderPass;

	VkCommandPool cmdPool;
	VkCommandPool presentCmdPool;
	VkCommandBuffer cmdBuffer; //used for initialization
	
	VkPipelineLayout pipelineLayout;
	VkPipelineCache pipelineCache;
	VkPipeline pipeline;

	Depth depth;

	VulkanManager(){} //do nothing
	~VulkanManager(){} //do nothing

	void startUp(Win32Window *window, VulkanConfig config, uint32 *demoWidth, uint32 *demoHeight);
	void shutDown();
	
	void prepareForResize();

	void displayInfo();
	void initDebugMessenger();
	void initInstance();
	void initSurface(Win32Window *window);
	void initPhysicalDevice(VkPhysicalDeviceFeatures featuresToEnable);
	void initCmdPool();
	void initDepthImage(VkFormat depthFormat, uint32 width, uint32 height);
	void initSyncPrimitives();
	
	void initBuffer(VkDeviceSize size, 
					VkBufferUsageFlags usageFlags, 
					VkMemoryPropertyFlags propertyFlags,
					VkBuffer &buffer, 
					VkDeviceMemory &bufferMemory);
					
	void initImage(uint32 width, uint32 height,
				   VkFormat format, VkImageTiling tiling, 
				   VkImageUsageFlags usage, VkMemoryPropertyFlags propertyFlags, 
				   VkImageLayout initialLayout, VkImage &image, VkDeviceMemory &imageMemory);

	void setImageLayout(VkImage image, 
					    VkImageAspectFlags aspectMask,
						VkImageLayout oldLayout, 
						VkImageLayout newLayout, 
						VkAccessFlagBits srcAccessMask, 
						VkPipelineStageFlags srcStages, 
						VkPipelineStageFlags destStages);

	void initVulkanTexture(uint8 *texPixels, 
						   uint32 texWidth,
						   uint32 texHeight,
						   VkBuffer &stagingBuffer, 
						   VkDeviceMemory &stagingBufferMemory, 
						   VulkanTexture &texture);
	
	void freeVulkanTexture(VulkanTexture &tex);
};

#endif