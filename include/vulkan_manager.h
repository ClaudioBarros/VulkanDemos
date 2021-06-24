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

	VkSurfaceKHR surface;

	VkInstance instance;

	PhysicalDevice physicalDevice;

	VulkanManager(){} //do nothing
	~VulkanManager(){} //do nothing

	void startUp(Win32Window &window, VulkanConfig config);
	void shutDown();

	void initInstance();
	void initSurface();
	void initPhysicalDevice();
};

struct VulkanConfig
{
	std::string appName;
	bool enableValidationLayers;
	std::string errorLogFilePath;
	
	std::vector<const char*> validationLayers;
	std::vector<const char*> deviceExtensions;
};

struct PhysicalDevice
{
	VkPhysicalDevice device;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;
	VkPhysicalDeviceMemoryProperties memProperties;
	
	std::vector<VkQueueFamilyProperties> queueFamilyProperties;

	PhysicalDevice(){}
	~PhysicalDevice(){}

	void init(VkPhysicalDevice physicalDevice)
	{
		this->device = physicalDevice;
		vkGetPhysicalDeviceProperties(device, &properties);
		vkGetPhysicalDeviceFeatures(device, &features);
		vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

		uint32 queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		queueFamilyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(this->device, 
		                                         &queueFamilyCount, 
												 queueFamilyProperties.data());
	}
	
	void destroy()
	{
		//implicitly destroyed when VkInstance is destroyed;
	}
};

#endif