#include "vulkan_manager.h"

void VulkanManager::startUp(Win32Window &window, VulkanConfig vulkanConfig)
{
    //INSTANCE CREATION
    this->config = vulkanConfig;

    //------ INSTANCE ---------

    initInstance();

    //------ SURFACE ---------------

    initSurface(window); 

    //----- PHYSICAL DEVICE ------------

    initPhysicalDevice();

    //---------- LOGICAL DEVICE AND QUEUES -------------

    logicalDevice.init(physicalDevice, config);
    //--------

    //TODO: SWAPCHAIN 
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();
    createDepthResources();
    createFramebuffers();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncPrimitives();

}

void VulkanManager::shutDown()
{
    //logical device
    logicalDevice.destroy();

    //surface
    vkDestroySurfaceKHR(instance, surface, nullptr);

    //instance
    vkDestroyInstance(instance, nullptr);
}

void VulkanManager::initSwapchain()
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;

    //query swapchain support details:
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
            throw std::runtime_error("FATAL_ERROR: Swapchain is not supported, Surface has no formats.");
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
            throw std::runtime_error("FATAL_ERROR: Swapchain is not supported, Surface has no present modes.");
        }
    }

    //select swapchain settings:
    VkSurfaceFormatKHR *selectedFormat = nullptr;
    VkPresentModeKHR selectedPresentMode;
    VkExtent2D selectedImageExtent = {};
    {
            //select surface format:
            bool wasSelected = false;
            for(VkSurfaceFormatKHR format : surfaceFormats)
            {
                //select SRGB if available
                if((format.format == VK_FORMAT_B8G8R8A8_SRGB) && 
                   (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR))
                {
                    selectedFormat = &format;
                    wasSelected = true;
                    break;
                }
            }
            if(!wasSelected) 
            {
                selectedFormat = &surfaceFormats[0];
            }

            //select present mode:
            wasSelected = false;
            for(VkPresentModeKHR mode : presentModes)
            {
                //select triple buffering if available
                if(mode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    selectedPresentMode = mode;
                    wasSelected = true;
                    break;
                }
            }
            if(!wasSelected)
            {
                selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR; 
            }
        
            //select image extent
            if(surfaceCapabilities.currentExtent.width != UINT32_MAX)
            {
                //UINT32_MAX acts as a flag to determine if the extent can
                //differ from the window resolution
                selectedImageExtent = surfaceCapabilities.currentExtent;
            }
            else
            {
                int width = 0;
                int height = 0;
                glfwGetFramebufferSize(window, &width, &height);

                uint32 widthExtent = (uint32)width;
                uint32 heightExtent = (uint32)height;

                widthExtent = std::max(surfaceCapabilities.minImageExtent.width,
                                       std::min(surfaceCapabilities.maxImageExtent.width, 
                                                widthExtent));

                heightExtent = std::max(surfaceCapabilities.minImageExtent.height,
                                       std::min(surfaceCapabilities.maxImageExtent.height, 
                                                heightExtent));
                
                selectedImageExtent.width = widthExtent;
                selectedImageExtent.height = heightExtent;
            }
    }

    //create swapchain:
    {
        uint32 imageCount = surfaceCapabilities.minImageCount + 1;
        if((surfaceCapabilities.maxImageCount > 0) && (imageCount > surfaceCapabilities.maxImageCount))
        {
            //maxImageCount = 0 means there is no maximum
            imageCount = surfaceCapabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = surface;
        swapchainCreateInfo.minImageCount = imageCount;
        swapchainCreateInfo.imageFormat = selectedFormat->format;
        swapchainCreateInfo.imageColorSpace = selectedFormat->colorSpace;
        swapchainCreateInfo.imageExtent = selectedImageExtent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32 queueFamilyIndices[] = {graphicsQueueFamilyIndex, presentQueueFamilyIndex};
        if(graphicsQueueFamilyIndex != presentQueueFamilyIndex)
        {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainCreateInfo.queueFamilyIndexCount = 2;
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchainCreateInfo.queueFamilyIndexCount = 0; //optional
            swapchainCreateInfo.pQueueFamilyIndices = nullptr; //optional
        }

        swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = selectedPresentMode;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = nullptr;

        if(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain)
           != VK_SUCCESS)
        {
            throw std::runtime_error("FATAL ERROR: unable to create swapchain.");
        }

        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());
    }

    //store relevant information in member variables
    swapchainImageFormat = selectedFormat->format;
    swapchainImageExtent = selectedImageExtent;

    //create an image view for each swapchain image:
    {
        swapchainImageViews.resize(swapchainImages.size());

        for(size_t i = 0; i < swapchainImages.size(); i++)
        {
            VkImageViewCreateInfo imageViewCreateInfo{};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.image = swapchainImages[i];
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = swapchainImageFormat;

            imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imageViewCreateInfo.subresourceRange.layerCount = 1;
            
            if(vkCreateImageView(device, &imageViewCreateInfo, 
                                 nullptr, &swapchainImageViews[i])   != VK_SUCCESS)
            {
                throw std::runtime_error("FATAL ERROR: Unable to create image views for the swapchain.");
            }

        }
    }
}

void VulkanManager::initPhysicalDevice()
{
    uint32 gpuCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));

    if(gpuCount <= 0)
    {
        LOGE_EXIT("VkEnumeratePhysicalDevice returned no accessible devie.");
    }

    std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data()));

    //try to select the best available GPU
    //look for a discrete gpu, if not found, settle for an integrated one, and so on.

    //VK_PHYSICAL_DEVICE_TYPE enums RANGE FROM 0 TO 4;
    std::vector<bool> deviceTypes((VK_PHYSICAL_DEVICE_TYPE_CPU + 1), false);
    
    VkPhysicalDeviceProperties deviceProperties;
    for(size_t i = 0; i < physicalDevices.size(); i++)
    {
        vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);
        deviceTypes[deviceProperties.deviceType] = true;
    }

    VkPhysicalDeviceType selectedType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;        
    if (deviceTypes[VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU]) 
    {
        selectedType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    } 
    else if (deviceTypes[VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU]) 
    {
        selectedType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    }
    else if (deviceTypes[VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU]) 
    {
        selectedType = VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU;
    } 
    else if (deviceTypes[VK_PHYSICAL_DEVICE_TYPE_CPU]) 
    {
        selectedType = VK_PHYSICAL_DEVICE_TYPE_CPU;
    } 
    else if (deviceTypes[VK_PHYSICAL_DEVICE_TYPE_OTHER]) 
    {
        selectedType = VK_PHYSICAL_DEVICE_TYPE_OTHER;
    }

    for(uint32_t i = 0; i < gpuCount; i++)
    {
        vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);
        if(deviceProperties.deviceType == selectedType)
        {
            this->physicalDevice.init(physicalDevices[i]);
            break;
        }
    }

    LOGI("Selected GPU: {}, type {}", 
         this->physicalDevice.properties.deviceName,
         this->physicalDevice.properties.deviceType);

}

void VulkanManager::initSurface(Win32Window &window)
{
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = window.handle;
    createInfo.hinstance = window.hInstance;

    VK_CHECK(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface));
}

void VulkanManager::initInstance()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = config.appName.data();
    appInfo.applicationVersion = 1;
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32 glfwExtensionCount = 0;
    const char** glfwExtensions;

    createInfo.enabledExtensionCount = (uint32)(config.deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = config.deviceExtensions.data();

    if(config.enableValidationLayers)
    {
        createInfo.enabledLayerCount = (uint32)(config.validationLayers.size());
        createInfo.ppEnabledLayerNames = config.validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));
}

