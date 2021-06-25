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
	


	VulkanManager(){} //do nothing
	~VulkanManager(){} //do nothing

	void startUp(Win32Window *window, VulkanConfig config);
	void shutDown();

	void initInstance();
	void initSurface(Win32Window *window);
	void initPhysicalDevice(VkPhysicalDeviceFeatures featuresToEnable);
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
	VkPhysicalDeviceFeatures supportedFeatures;
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

struct SwapchainImageResources 
{
    VkImage image;
    VkCommandBuffer cmd;
    VkCommandBuffer graphics_to_present_cmd;
    VkImageView view;
    VkBuffer uniform_buffer;
    VkDeviceMemory uniform_memory;
    void *uniform_memory_ptr;
    VkFramebuffer framebuffer;
    VkDescriptorSet descriptor_set;
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

	void chooseSettings(VkSurfaceFormatKHR preferredFormat,
						VkPresentModeKHR preferredPresentMode,
						uint32 width, uint32 height)
    {
		//select surface format:
		bool wasSelected = false;
		for(VkSurfaceFormatKHR f : surfaceFormats)
		{
			if((f.format == preferredFormat.format ) && 
				(f.colorSpace == preferredFormat.colorSpace))
			{
				surfaceFormat = f;
				wasSelected = true;
				break;
			}
		}
		//if the preferred format isn't available, choose the first one on the array.
		//TODO: find a better way to choose another format.
		if(!wasSelected) 
		{
			surfaceFormat = surfaceFormats[0];
			LOGI("Preferred swapchain format isn't available. "
			     "VK_PRESENT_MODE_FIFO_KHR selected instead");
		}

		//select present mode:
		wasSelected = false;
		for(VkPresentModeKHR mode : presentModes)
		{
			//select triple buffering if available
			if(mode == preferredPresentMode)
			{
				presentMode = mode;
				wasSelected = true;
				break;
			}
		}
		if(!wasSelected)
		{
			//this mode is guaranteed to be available
			presentMode = VK_PRESENT_MODE_FIFO_KHR; 
			LOGI("Preferred swapchain present mode isn't available. "
			     "VK_PRESENT_MODE_FIFO_KHR selected instead");
		}
	
		//select image extent
		if(surfaceCapabilities.currentExtent.width != 0xFFFFFFFF)
		{
			//if the surface size is defined (other than 0xFFFFFFFF), the swapchain size must match.
			imageExtent = surfaceCapabilities.currentExtent;
		}
		else
		{
			//if the surface size is undefined, the size should be set to the size of the requested
			//images, which should fit within the minimum and maximum values 

			imageExtent.width = width;
			imageExtent.height = height;

			if(width < surfaceCapabilities.minImageExtent.width)
			{
				imageExtent.width = surfaceCapabilities.minImageExtent.width;
			}
			else if(width > surfaceCapabilities.maxImageExtent.width)
			{
				imageExtent.width = surfaceCapabilities.maxImageExtent.width;
			}

			if(height < surfaceCapabilities.minImageExtent.height)
			{
				imageExtent.height = surfaceCapabilities.minImageExtent.height;
			}
			else if(height > surfaceCapabilities.maxImageExtent.height)
			{
				imageExtent.height = surfaceCapabilities.maxImageExtent.height;
			}
		}
    }

	void createSwapchainAndImageResources(VkSurfaceKHR surface, 
						 VkDevice logicalDevice)
	{
		VkSwapchainKHR oldSwapchain = swapchain;
        uint32 desiredNumImages = 3; //3 for triple-buffering

        if(desiredNumImages < surfaceCapabilities.minImageCount)
        {
            desiredNumImages = surfaceCapabilities.minImageCount;
        }
        if((surfaceCapabilities.maxImageCount > 0) && (desiredNumImages > surfaceCapabilities.maxImageCount))
        {
            //maxImageCount = 0 means there is no maximum
            desiredNumImages = surfaceCapabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = desiredNumImages;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = imageExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; //optional
		createInfo.pQueueFamilyIndices = nullptr; //optional
        createInfo.preTransform = surfaceCapabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.oldSwapchain = oldSwapchain;
        createInfo.clipped = VK_TRUE;

        VK_CHECK(vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapchain));

		
		// destroy the old swapchain if we are recreating the swapchain
		// Note: destroying the swapchain also cleans up all its associated
		// presentable images once the platform is done with them.
		if(oldSwapchain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(logicalDevice, oldSwapchain, NULL);
		}

		uint32 imageCount = 0;
		std::vector<VkImage> swapchainImages;
        vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
		imageResources.resize(imageCount);
        VK_CHECK(vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, swapchainImages.data()));

		for(uint32 i = 0; i < imageCount; i++)
        {
            VkImageViewCreateInfo imageViewCreateInfo{};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.image = swapchainImages[i];
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = surfaceFormat.format;

            imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imageViewCreateInfo.subresourceRange.layerCount = 1;
            
			imageResources[i].image = swapchainImages[i];
            VK_CHECK(vkCreateImageView(logicalDevice,
			                     	   &imageViewCreateInfo, 
								       nullptr,
								       &imageResources[i].view));
        }
	}

	public:

 	void init(VkPhysicalDevice physicalDevice,  
	 		  VkDevice logicalDevice,
			  VkSurfaceKHR surface,
			  VkSurfaceFormatKHR preferredFormat,
			  uint32 surfaceWidth, uint32 surfaceHeight)
	{
		querySupportInfo(physicalDevice, surface);
	}

	void destroy()
	{

	}
};

#endif