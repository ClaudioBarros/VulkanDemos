#include "vulkan_manager.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
/*
TODO LIST:
            -swapchain destruction
            -swapchain recreation
            -depth buffer 
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

    initPhysicalDevice(config.physDeviceFeaturesToEnable);

    //---------- LOGICAL DEVICE AND QUEUES -------------

    logicalDevice.init(physicalDevice, config);
    //--------

    swapchain.init(physicalDevice.device, 
                   logicalDevice.device, 
                   surface, 
                   config.preferredSurfaceFormat,
                   config.preferredPresentMode,
                   window->width, window->height);

    initRenderPass();

    initCmdPool();
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

void VulkanManager::initRenderPass()
{
    //Layout transitions:
    //color attachment: 
    //      -at the start of the render pass: UNDEFINED -> COLOR_ATTACHMENT_OPTIONAL.
    //      -at the start of the subpass COLOR_ATTACHMENT_OPTIONAL -> PRESENT_SRC_KHR;
    //
    //depth attachment:
    //      -at the start of the render pass: UNDEFINED -> DEPTH_STENCIL_ATTACHMENT_OPTIONAL.
    //
    //NOTE: This is all done as part ofthe renderpass, no barriers are necessary


    //color attachment:
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchain.surfaceFormat.format; //must match swapchain format
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; //not multisampled
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //clear to black at frame start
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //store the results when the frame ends
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; //not using a stencil buffer
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //not using a stencil buffer
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //image layout undefined at render pass start 
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;//transition layout to PRESENT_SRC_KHR 
                                                                    //when render pass is complete

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0; //index of our single attachment
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //depth attachment:
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = config.preferredDepthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment};

    //-------- Subpass -------
    //There will be a single subpass with a color and a depth attachment.

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    //need to wait for the Window System Integration semaphore to signal
    //"Only pipeline stages which depend on COLOR_ATTACHMENT_OUTPUT_BIT will
    // actually wait for the semaphore, so we must also wait for that pipeline stage."
    // src: official vulkan samples.


    VkSubpassDependency attachmentDependencies[2] = {};
    //[0]
    {
        // Depth buffer is shared between swapchain images
        attachmentDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL,
        attachmentDependencies[0].dstSubpass = 0,
        attachmentDependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | 
                                                 VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

        attachmentDependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | 
                                                 VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

        attachmentDependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        attachmentDependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | 
                                                  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        attachmentDependencies[0].dependencyFlags = 0;
    }
    //[1]
    {
        attachmentDependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
        attachmentDependencies[1].dstSubpass = 0;
        attachmentDependencies[1].srcStageMask =  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        attachmentDependencies[1].srcAccessMask = 0;
        attachmentDependencies[1].dstStageMask =  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        attachmentDependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    //create the render pass:
    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = (uint32)(sizeof(attachments) / sizeof(attachments[0])); 
    renderPassCreateInfo.pAttachments = attachments;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 2;
    renderPassCreateInfo.pDependencies = attachmentDependencies;

    VK_CHECK(vkCreateRenderPass(logicalDevice.device, &renderPassCreateInfo, nullptr, &renderPass));
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


void VulkanManager::initDepthBuffer(VkFormat depthFormat, uint32 width, uint32 height)
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
                              VkImage &image, VkDeviceMemory &imageMemory)
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
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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
    barrier.pNext = NULL;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = 0;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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

void VulkanManager::initTextures()
{
    for(uint32 i = 0; i < config.texCount; i++)
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(config.texFiles[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        
        //4 bytes per pixel;
        VkDeviceSize imgSize = texWidth * texHeight * 4;

        if(!pixels)
        {
            LOGE_EXIT("Unable to load texture image.");
        }

        initBuffer(imgSize,
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   stagingTexture.buffer, 
                   stagingTexture.mem); 

        void *data;
        vkMapMemory(logicalDevice.device, stagingTexture.mem, 0, imgSize, 0, &data);
        memcpy(data, pixels, (size_t)(imgSize));
        vkUnmapMemory(logicalDevice.device, stagingTexture.mem);

        stbi_image_free(pixels);

        initImage(texWidth, texHeight,
                  VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textures[i].image, textures[i].mem);

        setImageLayout(textures[i].image,
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
                               stagingTexture.buffer, 
                               textures[i].image, 
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &copyRegion);

        setImageLayout(textures[i].image,
                       VK_IMAGE_ASPECT_COLOR_BIT,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       textures[i].imageLayout, 
                       VK_ACCESS_TRANSFER_WRITE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    //TODO: create sampler
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

//=================================== Texture =========================================

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

void Swapchain::chooseSettings(VkSurfaceFormatKHR preferredFormat,
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

void Swapchain::init(VkPhysicalDevice physicalDevice,  
                     VkDevice logicalDevice,
                     VkSurfaceKHR surface,
                     VkSurfaceFormatKHR preferredFormat,
                     VkPresentModeKHR preferredPresentMode,
                    uint32 surfaceWidth, uint32 surfaceHeight)
{
    querySupportInfo(physicalDevice, surface);
    chooseSettings(preferredFormat, preferredPresentMode, surfaceWidth, surfaceHeight);
    createSwapchainAndImageResources(surface, logicalDevice);
}

void Swapchain::destroy()
{

}