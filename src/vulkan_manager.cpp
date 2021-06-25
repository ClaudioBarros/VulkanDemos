#include "vulkan_manager.h"

/*
TODO LIST:
            -swapchain destruction
            -swapchain recreation 
*/

void VulkanManager::startUp(Win32Window *window, VulkanConfig vulkanConfig)
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

    VkSurfaceFormatKHR preferredSurfaceFormat = { VK_FORMAT_R8G8B8A8_UNORM,
                                                  VK_COLORSPACE_SRGB_NONLINEAR_KHR};

    swapchain.init(physicalDevice.device, 
                   logicalDevice.device, 
                   surface, 
                   preferredSurfaceFormat,
                   window->width, window->height);

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

    //swapchain
    swapchain.destroy();

    //logical device
    logicalDevice.destroy();

    //surface
    vkDestroySurfaceKHR(instance, surface, nullptr);

    //instance
    vkDestroyInstance(instance, nullptr);
}


void VulkanManager::initPhysicalDevice(VkPhysicalDeviceFeatures featuresToEnable)
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
            this->physicalDevice.init(physicalDevices[i], surface, featuresToEnable);
            break;
        }
    }

    LOGI("Selected GPU: {}, type {}", 
         this->physicalDevice.properties.deviceName,
         this->physicalDevice.properties.deviceType);

}

void VulkanManager::initSurface(Win32Window *window)
{
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = window->handle;
    createInfo.hinstance = window->hInstance;

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

