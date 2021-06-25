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


	VulkanManager(){} //do nothing
	~VulkanManager(){} //do nothing

	void startUp(Win32Window *window, VulkanConfig config);
	void shutDown();

	void initInstance();
	void initSurface(Win32Window *window);
	void initPhysicalDevice(VkPhysicalDeviceFeatures featuresToEnable);
	void initRenderPass();
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
	
	std::vector<const char*> validationLayers;
	std::vector<const char*> deviceExtensions;
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

#endif