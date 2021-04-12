#include "vulkan_manager.h"
#include <stdexcept>

const std::vector<const char*> validationLayers = {
                                              "VK_LAYER_KHRONOS_validation"
                                            };
                                            
void VulkanManager::startUp()
{
    //INSTANCE CREATION
    #ifdef DEBUG
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
    for(uint32 i = 0; i < (uint32)(queueFamilyProperties.size()); i++)    
    {
       if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) 
       {
           graphicsQueueFamilyIndex = i;
           break;
       }
    }

    float graphicsQueuePriority = 1.0f;

    //---------- LOGICAL DEVICE AND QUEUES -------------
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &graphicsQueuePriority;

    //change this to enable device features when needed
    enabledFeatures = {};

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pEnabledFeatures = &enabledFeatures;

    //create logical device
    if(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL_ERROR: failed to create logical device.");
    }

    //get handle to the graphics queue 
    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
}

void VulkanManager::shutDown()
{
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
}

























