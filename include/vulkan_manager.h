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

	VulkanManager(){} //do nothing
	~VulkanManager(){} //do nothing

	void startUp(Win32Window &window, VulkanConfig config);
	void shutDown();

	void initInstance();
	void initSurface(Win32Window &window);
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
	VkPhysicalDeviceFeatures supportedfeatures;
	VkPhysicalDeviceFeatures enabledFeatures;
	VkPhysicalDeviceMemoryProperties memProperties;

	

	std::vector<VkQueueFamilyProperties> queueFamilyProperties;

	uint32 graphicsQueueFamilyIndex;
	uint32 presentQueueFamilyIndex;

	void init(VkPhysicalDevice physicalDevice, 
	          VkSurfaceKHR surface,
			  VkPhysicalDeviceFeatures featuresToEnable)
	{
		this->device = physicalDevice;
		vkGetPhysicalDeviceProperties(device, &properties);
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
		vkGetPhysicalDeviceMemoryProperties(device, &memProperties);
		enabledFeatures = featuresToEnable;

		uint32 queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		queueFamilyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(this->device, 
		                                         &queueFamilyCount, 
												 queueFamilyProperties.data());
		
		//Select a queue that supports a graphics queue 
		VkBool32 graphicsSupportFound = false;
		VkBool32 surfaceSuppportFound = false;

		for(uint32 i = 0; i < (uint32)(queueFamilyProperties.size()); i++)    
		{
			if((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				&& (graphicsSupportFound == false))
			{
				graphicsSupportFound = true;
				graphicsQueueFamilyIndex = i;
			}
			
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &surfaceSuppportFound);
			if(surfaceSuppportFound)
			{
				presentQueueFamilyIndex = i;
			}

			if(graphicsSupportFound && surfaceSuppportFound)
			{
				break;
			}
		}

	}

	void destroy()
	{
		//implicitly destroyed when VkInstance is destroyed;
	}
};

struct LogicalDevice
{
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	void init(PhysicalDevice &physicalDevice,
	          VulkanConfig config)
	{
		float queuePriority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = physicalDevice.graphicsQueueFamilyIndex;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueInfo);

		if(physicalDevice.graphicsQueueFamilyIndex != physicalDevice.presentQueueFamilyIndex)
		{
			queueInfo = {};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = physicalDevice.presentQueueFamilyIndex;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueInfo);
		}

		VkDeviceCreateInfo deviceInfo{};
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceInfo.queueCreateInfoCount = (uint32)(queueCreateInfos.size());
		deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceInfo.pEnabledFeatures = &physicalDevice.enabledFeatures;
		deviceInfo.enabledExtensionCount = (uint32)(config.deviceExtensions.size());
		deviceInfo.ppEnabledExtensionNames = config.deviceExtensions.data();

		if(config.enableValidationLayers)
		{
			deviceInfo.enabledLayerCount = (uint32)(config.validationLayers.size());
			deviceInfo.ppEnabledLayerNames = config.validationLayers.data();
		}

		//create logical device
	 	VK_CHECK(vkCreateDevice(physicalDevice.device, &deviceInfo, nullptr, &device));

		//get handles to the graphics  and presentation queues
		vkGetDeviceQueue(device, physicalDevice.graphicsQueueFamilyIndex, 0, &graphicsQueue);
		vkGetDeviceQueue(device, physicalDevice.presentQueueFamilyIndex, 0, &presentQueue);
	}

	void destroy()
	{
		vkDestroyDevice(device, nullptr);
	}
};

struct Swapchain
{
	VkSwapchainKHR swapchain;
	VkSurfaceCapabilitiesKHR surfaceCapabilities;	
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;

	private:

 	void querySupportInfo(VkPhysicalDevice physicalDevice,  
	  					  VkSurfaceKHR surface)
	{
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice,
                                                  surface,
                                                  &surfaceCapabilities);

        uint32 surfaceFormatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr);

        if(surfaceFormatCount != 0)
        {
            surfaceFormats.resize(surfaceFormatCount);

            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice,
                                                 surface,
                                                 &surfaceFormatCount,
                                                 surfaceFormats.data());
        }
        else
        {
            LOGE_EXIT("Swapchain is not supported, Surface has no formats.");
        }

        uint32 presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

        if(presentModeCount != 0)
        {
            presentModes.resize(presentModeCount);

            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice,
                                               surface,
                                               &presentModeCount,
                                               presentModes.data());
        }
        else
        {
            LOGE_EXIT("Swapchain is not supported. Surface has no present modes.");
        }
	}

	public:

	void init()
	{

	}

	void destroy()
	{

	}
};

#endif