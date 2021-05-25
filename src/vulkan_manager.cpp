#include "vulkan_manager.h"
#include <stdexcept>
#include <algorithm>

const std::vector<const char*> validationLayers = {
                                                    "VK_LAYER_KHRONOS_validation"
                                                  };
const std::vector<const char*> deviceExtensions = {
                                                    "VK_KHR_swapchain"
                                                  };
                                            
void VulkanManager::startUp(GLFWwindow *window)
{
    //INSTANCE CREATION
    #if _DEBUG
    bool enableValidationLayers = true;
    #else
    bool enableValidationLayers = false;
    #endif

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Demo";
    appInfo.applicationVersion = 1;
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32 glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    if(enableValidationLayers)
    {
        createInfo.enabledLayerCount = (uint32)(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL_ERROR: Could not create Vulkan Instance.");
    }

    //------ SURFACE ---------------

    if(glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL_ERROR: Unable to create window surface.");
    }

    //----- PHYSICAL DEVICE ------------

    uint32 gpuCount = 0;
    vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);
    if(gpuCount == 0)
    {
        throw std::runtime_error("FATAL_ERROR: No GPUs with Vulkan Support.");
    }

    std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
    if(vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL_ERROR: Unable to enumerate physical devices.");
    }

    //NOTE: for now, the first device enumerated is selected. 
    //TODO: change this to select the best available GPU.
    physicalDevice = physicalDevices[0];
    
    //Store device properties
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

    //-------- QUEUE FAMILIES ------------------
    //queue family properties, used for setting up requested queues upon device creation
    uint32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    if(queueFamilyCount == 0)
    {
        throw std::runtime_error("FATAL_ERROR: No queue families were found.");
    }

    queueFamilyProperties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                             &queueFamilyCount,
                                             queueFamilyProperties.data());

    //Select a queue that supports a graphics queue 
    uint32 graphicsQueueFamilyIndex;
    uint32 presentQueueFamilyIndex;

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

    //---------- LOGICAL DEVICE AND QUEUES -------------

    float queuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueInfo);

    if(graphicsQueueFamilyIndex != presentQueueFamilyIndex)
    {
        queueInfo = {};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = presentQueueFamilyIndex;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueInfo);
    }

    //change this to enable device features when needed
    enabledFeatures = {};

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = (uint32)(queueCreateInfos.size());
    deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceInfo.pEnabledFeatures = &enabledFeatures;
    deviceInfo.enabledExtensionCount = (uint32)(deviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
    if(enableValidationLayers)
    {
        deviceInfo.enabledLayerCount = (uint32)(validationLayers.size());
        deviceInfo.ppEnabledLayerNames = validationLayers.data();
    }

    //create logical device
    if(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL_ERROR: failed to create logical device.");
    }

    //get handles to the graphics  and presentation queues
    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);

    //-------------------- SWAP-CHAIN --------------------

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
    }

    //check for swapchain support 
    if(surfaceFormats.empty() || presentModes.empty())
    {
        throw std::runtime_error("FATAL_ERROR: Swapchain is not supported.");
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
        if((surfaceCapabilities.maxImageCount > 0) && (surfaceCapabilities.maxImageCount))
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
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

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
    swapchainImageFormat = selectedFormat;
    swapchainImageExtent = selectedImageExtent;
    

}

void VulkanManager::shutDown()
{
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}
