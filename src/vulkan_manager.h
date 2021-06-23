#ifndef VULKAN_MANAGER_H
#define VULKAN_MANAGER_H

#include "vulkan/vulkan.h"
#include <vector>
#include <string>
#include "platform.h"
#include "typedefs_and_macros.h"

struct VulkanManager
{
	VulkanConfig config;

	VkInstance instance;

	VulkanManager(){} //do nothing
	~VulkanManager(){} //do nothing

	void startUp(Window window, VulkanConfig config);
	void shutDown();

	void initInstance();
	void initSurface();
};

struct VulkanConfig
{
	std::string appName;
	bool enableValidationLayers;
	std::string errorLogFilePath;
	
	std::vector<const char*> validationLayers;
	std::vector<const char*> deviceExtensions;
};

#endif