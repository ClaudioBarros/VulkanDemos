#include "vulkan_manager.h"

void VulkanManager::startUp(Win32Window *window, 
                            VulkanConfig vulkanConfig, 
                            uint32 *demoWidth, 
                            uint32 *demoHeight)
{
    this->config = vulkanConfig;
    displayInfo();

    //------ INSTANCE ---------

    initInstance();

    //------ Debug Callback -----
    initDebugMessenger();

    //------ SURFACE ---------------

    initSurface(window); 

    //----- PHYSICAL DEVICE ------------

    initPhysicalDevice(config.physDeviceFeaturesToEnable);

    //---------- LOGICAL DEVICE AND QUEUES -------------

    logicalDevice.init(physicalDevice, config);

    //-------------- SYNC PRIMITIVES ----------------
    initSyncPrimitives();

    //--------------- SWAPCHAIN ------------------
    swapchain.swapchain = VK_NULL_HANDLE;
    swapchain.queryInfoAndChooseSettings(physicalDevice.device, 
                                         logicalDevice.device, surface, 
                                         config.preferredSurfaceFormat,
                                         config.preferredPresentMode,
                                         demoWidth, demoHeight);

    //--------------- COMMAND POOL ---------------
    initCmdPool();
}

void VulkanManager::shutDown()
{
    //Wait for fences from present operations
    for(uint32 i = 0; i < MAX_FRAMES; i++)
    {
        vkWaitForFences(logicalDevice.device, 1, 
                        &fences[i], VK_TRUE, UINT64_MAX);

        vkDestroyFence(logicalDevice.device, fences[i], nullptr);
        vkDestroySemaphore(logicalDevice.device, imageAcquiredSemaphores[i], nullptr);
        vkDestroySemaphore(logicalDevice.device, drawCompleteSemaphores[i], nullptr);
        
        if(physicalDevice.separatePresentQueue)
        {
            vkDestroySemaphore(logicalDevice.device, imageOwnershipSemaphores[i], nullptr);
        }
    }

    //if the window is minimized, prepareForResize() has already done some cleanup.
    if(!(*isMinimized))
    {
        vkDestroyPipeline(logicalDevice.device, pipeline, nullptr);
        vkDestroyPipelineCache(logicalDevice.device, pipelineCache, nullptr);
        vkDestroyPipelineLayout(logicalDevice.device, pipelineLayout, nullptr); 

        vkDestroyRenderPass(logicalDevice.device, renderPass, nullptr);

        vkDestroyImageView(logicalDevice.device, depth.view, nullptr);
        vkDestroyImage(logicalDevice.device, depth.image, nullptr);
        vkFreeMemory(logicalDevice.device, depth.mem, nullptr);
        
        swapchain.destroy(logicalDevice.device, cmdPool);
        
        vkDestroyCommandPool(logicalDevice.device, cmdPool, nullptr);
        if(physicalDevice.separatePresentQueue)
        {
            vkDestroyCommandPool(logicalDevice.device, presentCmdPool, nullptr);
        }
     }

    vkDeviceWaitIdle(logicalDevice.device);

    logicalDevice.destroy();

    vkDestroySurfaceKHR(instance, surface, nullptr);
    
    if(config.enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroyInstance(instance, nullptr);
}

void VulkanManager::prepareForResize()
{
    vkDestroyPipeline(logicalDevice.device, pipeline, nullptr);
    vkDestroyPipelineCache(logicalDevice.device, pipelineCache, nullptr);
    vkDestroyRenderPass(logicalDevice.device, renderPass, nullptr);
    vkDestroyPipelineLayout(logicalDevice.device, pipelineLayout, nullptr); 

    vkDestroyImageView(logicalDevice.device, depth.view, nullptr);
    vkDestroyImage(logicalDevice.device, depth.image, nullptr);
    vkFreeMemory(logicalDevice.device, depth.mem, nullptr);

    swapchain.destroy(logicalDevice.device, cmdPool);
    
    vkDestroyCommandPool(logicalDevice.device, cmdPool, nullptr);
    if(physicalDevice.separatePresentQueue)
    {
        vkDestroyCommandPool(logicalDevice.device, presentCmdPool, nullptr);
    }
}

void VulkanManager::displayInfo()
{
    uint32 extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    std::string availableExtensionsStr = "\n\nAvailable Extensions: ";
    for(VkExtensionProperties &extension : extensions)
    {
        availableExtensionsStr.append("\n\t");
        availableExtensionsStr.append(extension.extensionName);
        availableExtensionsStr.append("\n");
    }
    
    LOGI(availableExtensionsStr);
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
                                      const VkAllocationCallbacks* pAllocator, 
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) 
                vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr) 
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } 
    else 
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, 
                                   VkDebugUtilsMessengerEXT debugMessenger, 
                                   const VkAllocationCallbacks* pAllocator) 
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) 
                 vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr) 
    {
        func(instance, debugMessenger, pAllocator);
    }
}

void VulkanManager::initDebugMessenger()
{
    if(!config.enableValidationLayers || (config.ptrDebugMessenger == nullptr))
    {
        return;
    }
    
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    createInfo.pfnUserCallback = config.ptrDebugMessenger;

    VK_CHECK(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger));
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
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    createInfo.enabledExtensionCount = (uint32)(config.instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = config.instanceExtensions.data();

    if(config.enableValidationLayers)
    {
        createInfo.enabledLayerCount = (uint32)(config.validationLayers.size());
        createInfo.ppEnabledLayerNames = config.validationLayers.data();

        VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo;
        debugMessengerInfo = {};
        debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

        debugMessengerInfo.messageSeverity = 
                                             //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                                         VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        debugMessengerInfo.pfnUserCallback = config.ptrDebugMessenger;
        
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugMessengerInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));
}

void VulkanManager::initCmdPool()
{
    VkCommandPoolCreateInfo cmdPoolInfo{};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.pNext = nullptr;
    cmdPoolInfo.queueFamilyIndex = physicalDevice.graphicsQueueFamilyIndex;
    cmdPoolInfo.flags = 0;

    VK_CHECK(vkCreateCommandPool(logicalDevice.device, &cmdPoolInfo, nullptr, &cmdPool));
}

uint32 findMemoryTypeFromProperties(const VkPhysicalDeviceMemoryProperties *pMemoryProperties,
                                    uint32 memoryTypeBitsRequired, 
                                    VkMemoryPropertyFlags requiredProperties)
{
    uint32 memoryCount = pMemoryProperties->memoryTypeCount;
    for(uint32 memoryIndex = 0; memoryIndex < memoryCount; ++memoryIndex) 
    {
        uint32 memoryTypeBits = (1 << memoryIndex);
        bool isRequiredMemoryType = memoryTypeBitsRequired & memoryTypeBits;

        VkMemoryPropertyFlags properties =
            pMemoryProperties->memoryTypes[memoryIndex].propertyFlags;
        bool hasRequiredProperties =
            (properties & requiredProperties) == requiredProperties;

        if (isRequiredMemoryType && hasRequiredProperties)
            return memoryIndex;
    }

    LOGE_EXIT("Unable to find suitable memory type from the given properties.");
}

void VulkanManager::initDepthImage(VkFormat depthFormat, uint32 width, uint32 height)
{
    //--- create image --- 

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = depthFormat;
    imageInfo.extent = {width, height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VK_CHECK(vkCreateImage(logicalDevice.device, &imageInfo, nullptr, &depth.image));

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(logicalDevice.device, depth.image, &memReqs);

    depth.memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    depth.memAlloc.pNext = nullptr;
    depth.memAlloc.allocationSize = memReqs.size;
    depth.memAlloc.memoryTypeIndex = findMemoryTypeFromProperties(&physicalDevice.memProperties,
                                                                  memReqs.memoryTypeBits,
                                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // --- allocate memory ---
    VK_CHECK(vkAllocateMemory(logicalDevice.device, &depth.memAlloc, nullptr, &depth.mem));

    // --- bind memory ---
    VK_CHECK(vkBindImageMemory(logicalDevice.device, depth.image, depth.mem, 0));

    // --- create image view ---
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depth.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(logicalDevice.device, &viewInfo, nullptr, &depth.view));
}

void VulkanManager::initSyncPrimitives()
{
    //Create semaphores to sunchronize acquiring presentable buffers before rendering
    //and waiting for drawaing to be complete before presenting
    
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    //Create fences that we can use to throttle if we get too far ahead of the image presents
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for(uint32 i = 0; i < MAX_FRAMES; i++)
    {
        VK_CHECK(vkCreateFence(logicalDevice.device, &fenceInfo, nullptr, &fences[i]));

        VK_CHECK(vkCreateSemaphore(logicalDevice.device, 
                                   &semaphoreInfo, 
                                   nullptr, 
                                   &imageAcquiredSemaphores[i]));

        VK_CHECK(vkCreateSemaphore(logicalDevice.device, 
                                   &semaphoreInfo, 
                                   nullptr, 
                                   &drawCompleteSemaphores[i]));
        
        if(physicalDevice.separatePresentQueue)
        {
            VK_CHECK(vkCreateSemaphore(logicalDevice.device, 
                                       &semaphoreInfo, 
                                       nullptr, 
                                       &imageOwnershipSemaphores[i]));
        }
    }
}

void VulkanManager::initBuffer(VkDeviceSize size, 
                               VkBufferUsageFlags usageFlags, 
                               VkMemoryPropertyFlags propertyFlags,
                               VkBuffer &buffer, 
                               VkDeviceMemory &bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateBuffer(logicalDevice.device, &bufferInfo, nullptr, &buffer));

    //memory allocation for the buffer:
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(logicalDevice.device, buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryTypeFromProperties(&physicalDevice.memProperties,
                                                             memReqs.memoryTypeBits,
                                                             propertyFlags);

    VK_CHECK(vkAllocateMemory(logicalDevice.device, &allocInfo, nullptr, &bufferMemory));

    VK_CHECK(vkBindBufferMemory(logicalDevice.device, buffer, bufferMemory, 0));
}

void VulkanManager::initImage(uint32 width, uint32 height,
                              VkFormat format, VkImageTiling tiling, 
                              VkImageUsageFlags usage, VkMemoryPropertyFlags propertyFlags, 
                              VkImageLayout initialLayout, VkImage &image, 
                              VkDeviceMemory &imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = initialLayout;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateImage(logicalDevice.device, &imageInfo, nullptr, &image));

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(logicalDevice.device, image, &memReqs);
     
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryTypeFromProperties(&physicalDevice.memProperties,
                                                              memReqs.memoryTypeBits,
                                                              propertyFlags);

    VK_CHECK(vkAllocateMemory(logicalDevice.device, &allocInfo, nullptr, &imageMemory));

    VK_CHECK(vkBindImageMemory(logicalDevice.device, image, imageMemory, 0));
}

void VulkanManager::setImageLayout(VkImage image, 
                                   VkImageAspectFlags aspectMask,
                                   VkImageLayout oldLayout, 
                                   VkImageLayout newLayout, 
                                   VkAccessFlagBits srcAccessMask, 
                                   VkPipelineStageFlags srcStages, 
                                   VkPipelineStageFlags destStages) 
{

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = 0;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    switch (newLayout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            /* Make sure anything that was copying from this image has completed */
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            break;

        default:
            barrier.dstAccessMask = 0;
            break;
    }

    vkCmdPipelineBarrier(cmdBuffer, srcStages, destStages, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanManager::initVulkanTexture(uint8 *texPixels, 
                                      uint32 texWidth,
                                      uint32 texHeight,
                                      VkBuffer &stagingBuffer, 
                                      VkDeviceMemory &stagingBufferMemory, 
                                      VulkanTexture &texture)
{
    assert((texPixels != nullptr) && (texWidth > 0) && (texWidth > 0));

    //4 bytes per pixel;
    VkDeviceSize imgSize = texWidth * texHeight * 4;

    if(!texPixels)
    {
        LOGE_EXIT("Unable to initialize Vulkan Texture. parameter texPixels was nullptr.");
    }

    initBuffer(imgSize,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, 
               stagingBufferMemory); 

    void *data;
    vkMapMemory(logicalDevice.device, stagingBufferMemory, 0, imgSize, 0, &data);
    memcpy(data, texPixels, (size_t)(imgSize));
    vkUnmapMemory(logicalDevice.device, stagingBufferMemory);

    initImage(texWidth, texHeight,
              config.texFormat, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_LAYOUT_UNDEFINED, 
              texture.image, texture.mem);
    
    texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    setImageLayout(texture.image,
                   VK_IMAGE_ASPECT_COLOR_BIT,
                   VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                   VK_ACCESS_NONE_KHR,
                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                   VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkBufferImageCopy copyRegion{};
    copyRegion.bufferOffset = 0; 
    copyRegion.bufferRowLength = 0; //means texture is tighly packed
    copyRegion.bufferImageHeight = 0; //means texture is tighly packed
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;

    copyRegion.imageOffset = {0, 0, 0};
    copyRegion.imageExtent = {(uint32)texWidth, (uint32)texHeight, 1};


    vkCmdCopyBufferToImage(cmdBuffer, 
                           stagingBuffer, 
                           texture.image, 
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &copyRegion);

    setImageLayout(texture.image,
                   VK_IMAGE_ASPECT_COLOR_BIT,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   texture.imageLayout, 
                   VK_ACCESS_TRANSFER_WRITE_BIT,
                   VK_PIPELINE_STAGE_TRANSFER_BIT,
                   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    //--- sampler ---
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = physicalDevice.properties.limits.maxSamplerAnisotropy;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    //--- image view ----
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = config.texFormat;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    VK_CHECK(vkCreateSampler(logicalDevice.device, &samplerInfo, nullptr, &texture.sampler));
    VK_CHECK(vkCreateImageView(logicalDevice.device, &viewInfo, nullptr, &texture.view));
}

void VulkanManager::freeVulkanTexture(VulkanTexture &tex)
{
    if(tex.sampler) vkDestroySampler(logicalDevice.device, tex.sampler, nullptr);
    if(tex.view) vkDestroyImageView(logicalDevice.device, tex.view, nullptr);
    if(tex.image) vkDestroyImage(logicalDevice.device, tex.image, nullptr);
    if(tex.buffer) vkDestroyBuffer(logicalDevice.device, tex.buffer, nullptr);
    if(tex.mem) vkFreeMemory(logicalDevice.device, tex.mem, nullptr);
}

//==================================== PhysicalDevice ===================================

void PhysicalDevice::init(VkPhysicalDevice physicalDevice, 
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
    
    if(graphicsQueueFamilyIndex != presentQueueFamilyIndex)
    {
        separatePresentQueue = false;
    }
    else
    {
        separatePresentQueue = true;
    }

}

void PhysicalDevice::destroy()
{
    //implicitly destroyed when VkInstance is destroyed;
}

//==================================== LogicalDevice ===================================
void LogicalDevice::init(PhysicalDevice &physicalDevice,
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

void LogicalDevice::destroy()
{
    vkDestroyDevice(device, nullptr);
}

//=================================== Swapchain =========================================
void Swapchain::querySupportInfo(VkPhysicalDevice physicalDevice,  
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
        LOGE_EXIT("Swapchain is not supported. Surface has no formats.");
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

void Swapchain::chooseSettings(VkSurfaceFormatKHR preferredFormat,
                               VkPresentModeKHR preferredPresentMode,
                               uint32 *demoWidth, uint32 *demoHeight)
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
        *demoWidth = surfaceCapabilities.currentExtent.width;
        *demoHeight = surfaceCapabilities.currentExtent.height;
    }
    else
    {
        //if the surface size is undefined, the size should be set to the size of the requested
        //images, which should fit within the minimum and maximum values 

        imageExtent.width = *demoWidth;
        imageExtent.height = *demoHeight;

        if(imageExtent.width < surfaceCapabilities.minImageExtent.width)
        {
            imageExtent.width = surfaceCapabilities.minImageExtent.width;
        }
        else if(imageExtent.width > surfaceCapabilities.maxImageExtent.width)
        {
            imageExtent.width = surfaceCapabilities.maxImageExtent.width;
        }

        if(imageExtent.height < surfaceCapabilities.minImageExtent.height)
        {
            imageExtent.height = surfaceCapabilities.minImageExtent.height;
        }
        else if(imageExtent.height > surfaceCapabilities.maxImageExtent.height)
        {
            imageExtent.height = surfaceCapabilities.maxImageExtent.height;
        }
    }    
}

void Swapchain::createSwapchainAndImageResources(VkSurfaceKHR surface, 
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

void Swapchain::queryInfoAndChooseSettings(VkPhysicalDevice physicalDevice,  
                                           VkDevice logicalDevice,
                                           VkSurfaceKHR surface,
                                           VkSurfaceFormatKHR preferredFormat,
                                           VkPresentModeKHR preferredPresentMode,
                                           uint32 *demoWidth, uint32 *demoHeight)
{
    querySupportInfo(physicalDevice, surface);
    chooseSettings(preferredFormat, preferredPresentMode, demoWidth, demoHeight);
}

void Swapchain::destroy(VkDevice &device, VkCommandPool &cmdPool)
{
    for(size_t i = 0; i < imageResources.size(); i++)
    {
        vkDestroyFramebuffer(device, imageResources[i].framebuffer, nullptr);

        vkDestroyImageView(device, imageResources[i].view, nullptr);
        
        vkFreeCommandBuffers(device, cmdPool, 1, &imageResources[i].cmd);
        
        vkDestroyBuffer(device, imageResources[i].uniformBuffer, nullptr);
        
        vkUnmapMemory(device, imageResources[i].uniformMemory);
        vkFreeMemory(device, imageResources[i].uniformMemory, nullptr);
    }
}




























