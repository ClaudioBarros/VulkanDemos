#ifndef VULKAN_MANAGER_H
#define VULKAN_MANAGER_H

#include "vulkan/vulkan.h"
#include <vector>
#include <string>

#include "typedefs_and_macros.h"

struct VulkanManager
{
	VulkanConfig config;

	VkInstance instance;

	VulkanManager(){} //do nothing
	~VulkanManager(){} //do nothing

	void startUp(VulkanConfig config);
	void shutDown();

	void initInstance();
};

struct VulkanConfig
{
	bool enableValidationLayers;
	std::string errorLogFilePath;
	
	std::vector<const char*> validationLayers;
	std::vector<const char*> deviceExtensions;
};

#endif