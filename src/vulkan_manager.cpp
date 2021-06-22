#include "vulkan_manager.h"

void VulkanManager::startUp(VulkanConfig vulkanConfig)
{
    
    //INSTANCE CREATION
    this->config = vulkanConfig;

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

    VkResult result =  
    if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL_ERROR: Unable to create Vulkan Instance.");
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
    enabledFeatures.samplerAnisotropy = VK_TRUE;

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

    //--------
    createSwapchain();
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

void VulkanManager::initInstance();
void VulkanManager::shutDown()
{

}