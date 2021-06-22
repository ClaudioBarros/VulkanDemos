#include "vulkan_manager.h"
#include <stdexcept>
#include <algorithm>
#include <fstream>
#include "vertex_formats.h"

#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <chrono>

const std::vector<const char*> validationLayers = {
                                                    "VK_LAYER_KHRONOS_validation"
                                                  };
const std::vector<const char*> deviceExtensions = {
                                                    "VK_KHR_swapchain"
                                                  };
                                            

const int MAX_FRAMES = 3;

const std::vector<Vertex> vertices =
{
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<uint32> indices = 
{
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

uint32 VulkanManager::getMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for(uint32 i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if((typeFilter & (1 << i)) && 
          ((memProperties.memoryTypes[i].propertyFlags & properties) == properties))
        {
            return i;
        }
    }

    throw std::runtime_error("FATAL ERROR: Unable to get suitable memory type from the physical device.");
}

void VulkanManager::createBuffer(VkDeviceSize size, 
                    VkBufferUsageFlags usageFlags, 
                    VkMemoryPropertyFlags propertyFlags,
                    VkBuffer &buffer, VkDeviceMemory &bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL ERROR: Unable to create buffer.");
    }

    //memory allocation for the buffer:
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = getMemoryType(memRequirements.memoryTypeBits, propertyFlags);

    if(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL ERROR: Unable to allocate buffer memory.");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void VulkanManager::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; //optional
    copyRegion.dstOffset = 0; //optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

void VulkanManager::copyBufferToImage(VkBuffer buffer, VkImage image, uint32 width, uint32 height)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy copyRegion{};
    copyRegion.bufferOffset = 0; 
    copyRegion.bufferRowLength = 0; 
    copyRegion.bufferImageHeight = 0;
    
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;

    copyRegion.imageOffset = {0, 0, 0};
    copyRegion.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, 
                           buffer, image, 
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,  &copyRegion);
    
    endSingleTimeCommands(commandBuffer);
}

void VulkanManager::transitionImageLayout(VkImage image, VkFormat format, 
                                          VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) &&
       (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL))
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if((oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) &&
           (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        throw std::invalid_argument("FATAL ERROR: Unsupported layout transition.");
    }

    vkCmdPipelineBarrier(commandBuffer,
                         sourceStage, destinationStage, 
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    endSingleTimeCommands(commandBuffer);
}

void VulkanManager::createImage(uint32 width, uint32 height,
                                VkFormat format, VkImageTiling tiling, 
                                VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
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

    if(vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL ERROR: Unable to create Vulkan Image.");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);
     
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = getMemoryType(memRequirements.memoryTypeBits, properties);

    if(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL ERROR: Unable to allocate Vulkan Image memory.");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

void VulkanManager::startUp(GLFWwindow *window)
{
    this->window = window;

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

void VulkanManager::createSwapchain()
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

void VulkanManager::createRenderPass()
{
    //color buffer
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat; //must match swapchain format
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; //not multisampled
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //clear to black at frame start
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //store the results when the frame ends
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; //not using a stencil buffer
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //not using a stencil buffer
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //image layout undefined at render pass start 
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;//transition layout to PRESENT_SRC_KHR 
                                                                    //when render pass is complete

    //there will be two layout transitions:
    //UNDEFINED -> COLOR_ATTACHMENT_OPTIONAL.
    //COLOR_ATTACHMENT_OPTIONAL -> PRESENT_SRC_KHR;
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0; //index of our single attachment
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //depth buffer
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
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
    //Create subpass
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
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                             | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

    dependency.srcAccessMask = 0;
    dependency.dstStageMask =  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                             | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                             | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


    //create the render pass:
    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = (uint32)(sizeof(attachments) / sizeof(attachments[0])); 
    renderPassCreateInfo.pAttachments = attachments;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dependency;

    if(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL ERROR: Unable to create render pass.");
    }
}

void VulkanManager::createDescriptorPool()
{
    VkDescriptorPoolSize poolSizes[2];

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = (uint32)(swapchainImages.size());
    
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = (uint32)(swapchainImages.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = &poolSizes[0];
    poolInfo.maxSets = (uint32)(swapchainImages.size());

    if(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL ERROR: Unable to create descriptor pool.");
    }
}

void VulkanManager::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding bindings[] = {uboLayoutBinding, samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = &bindings[0];

    if(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL ERROR: unable to create descriptor set layout.");
    }
}

void VulkanManager::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(swapchainImages.size(), descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = (uint32)(swapchainImages.size());
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(swapchainImages.size());
    if(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data())!= VK_SUCCESS)
    {
        throw std::runtime_error("FATAL ERROR: Unable to allocate descriptor sets.");
    }
    
    VkWriteDescriptorSet  descriptorWrites[2] = {};
    for(size_t i = 0; i < swapchainImages.size(); i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImgView;
        imageInfo.sampler = textureSampler;

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0; 
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0; 
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo; 

        vkUpdateDescriptorSets(device, 2, &descriptorWrites[0], 0, nullptr);
    }
}


void VulkanManager::createGraphicsPipeline()
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

    if(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout)
        != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL_ERROR: Unable to create pipeline.");
    }

    //vertex binding description 
    VkVertexInputBindingDescription vertexBindingDescription{};
    vertexBindingDescription.binding = 0;
    vertexBindingDescription.stride = sizeof(Vertex);
    vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    //vertex position attachment description
    VkVertexInputAttributeDescription posAttributeDescription{};
    posAttributeDescription.binding = 0;
    posAttributeDescription.location = 0;
    posAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
    posAttributeDescription.offset = offsetof(Vertex, pos);

    //vertex color attachment description 
    VkVertexInputAttributeDescription colorAttributeDescription{};
    colorAttributeDescription.binding = 0;
    colorAttributeDescription.location = 1;
    colorAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorAttributeDescription.offset = offsetof(Vertex, color);

    //vertex texture coordinate description
    VkVertexInputAttributeDescription texCoordAttributeDescription{};
    texCoordAttributeDescription.binding = 0;
    texCoordAttributeDescription.location = 2;
    texCoordAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
    texCoordAttributeDescription.offset = offsetof(Vertex, texCoord);

    VkVertexInputAttributeDescription vertexAttributeDescriptions[] = {posAttributeDescription, 
                                                                       colorAttributeDescription,
                                                                       texCoordAttributeDescription};

    uint32 attributeCount = (uint32)(sizeof(vertexAttributeDescriptions) / 
                                     sizeof(vertexAttributeDescriptions[0]));

    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = attributeCount;
    vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription; 
    vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions;


    //specify triangle lists as topology to draw geometry
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    //specify rasterization state
    VkPipelineRasterizationStateCreateInfo rasterCreateInfo{};
    rasterCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterCreateInfo.lineWidth = 1.0f;

    //attachment will write to all color channels
    //no blending enabled
    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
                                        VK_COLOR_COMPONENT_G_BIT |
                                        VK_COLOR_COMPONENT_B_BIT |
                                        VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blendStateCreateInfo{};
    blendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendStateCreateInfo.attachmentCount = 1;
    blendStateCreateInfo.pAttachments = &blendAttachment;


    //specify that one viewport and one scissor box will be used
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.scissorCount = 1;

    //disable depth and stencil testing
    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{};
    depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilCreateInfo.minDepthBounds = 0.0f;
    depthStencilCreateInfo.maxDepthBounds = 1.0f;
    depthStencilCreateInfo.stencilTestEnable = VK_FALSE;
    depthStencilCreateInfo.front = {};
    depthStencilCreateInfo.back = {};

    //specify no multisampling
    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo{};
    multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    //Specify that the viewport and scissor states will be dynamic.
    std::vector<VkDynamicState> dynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicStatesCreateInfo{};
    dynamicStatesCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStatesCreateInfo.pDynamicStates = dynamicStates.data();
    dynamicStatesCreateInfo.dynamicStateCount = (uint32)(dynamicStates.size());

    std::vector<char> vertShaderFileBuffer;
    std::vector<char> fragShaderFileBuffer;
    
    //Load shader modules:
    {
        std::string vertShaderFilename = "shaders/triangle/triangle_vert.spv";
        std::string fragShaderFilename = "shaders/triangle/triangle_frag.spv";

        //start reading at the end of the file to be able to determine file size 
        //spir-v files need to be read in binary mode
        std::ifstream vertShaderFile(vertShaderFilename, std::ios::ate | std::ios::binary);
        std::ifstream fragShaderFile(fragShaderFilename, std::ios::ate | std::ios::binary);

        if(!vertShaderFile.is_open())
        {
            throw std::runtime_error("FATAL_ERROR: Unable to open vertex shader file.");
        }
        if(!fragShaderFile.is_open())
        {
            throw std::runtime_error("FATAL_ERROR: Unable to open fragment shader file.");
        }

        size_t vertShaderFilesize = (size_t)vertShaderFile.tellg();
        size_t fragShaderFilesize = (size_t)fragShaderFile.tellg();
        vertShaderFileBuffer.resize(vertShaderFilesize);
        fragShaderFileBuffer.resize(fragShaderFilesize);
        
        vertShaderFile.seekg(0);
        fragShaderFile.seekg(0);

        vertShaderFile.read(vertShaderFileBuffer.data(), vertShaderFilesize);
        fragShaderFile.read(fragShaderFileBuffer.data(), fragShaderFilesize);

        vertShaderFile.close();
        fragShaderFile.close();
    }        

    VkShaderModuleCreateInfo vertShaderCreateInfo{};
    vertShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertShaderCreateInfo.codeSize = vertShaderFileBuffer.size();
    vertShaderCreateInfo.pCode = reinterpret_cast<const uint32*>(vertShaderFileBuffer.data());

    VkShaderModuleCreateInfo fragShaderCreateInfo{};
    fragShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragShaderCreateInfo.codeSize = fragShaderFileBuffer.size();
    fragShaderCreateInfo.pCode = reinterpret_cast<const uint32*>(fragShaderFileBuffer.data());
    
    //create shader modules 
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    
    if(vkCreateShaderModule(device, &vertShaderCreateInfo, nullptr, &vertShaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL_ERROR: Unable to create vertex shader module.");
    }

    if(vkCreateShaderModule(device, &fragShaderCreateInfo, nullptr, &fragShaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL_ERROR: Unable to create vertex shader module.");
    }

    VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
    vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageCreateInfo.module = vertShaderModule;
    vertShaderStageCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
    fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageCreateInfo.module = fragShaderModule;
    fragShaderStageCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageCreateInfo, fragShaderStageCreateInfo};

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2; //2 -> vertex and shader stages
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterCreateInfo;
    pipelineCreateInfo.pColorBlendState = &blendStateCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStatesCreateInfo;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.layout = pipelineLayout;

    if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline)
        != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL_ERROR: Unable to create graphics pipeline.");
    }

    //shader modules are safe to destroy after the graphics pipeline is created
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void VulkanManager::createFramebuffers()
{
    //create a framebuffer for each swapchain image view 
    swapchainFramebuffers.resize(swapchainImageViews.size());

    VkImageView attachments[2];
    for(size_t i = 0; i < swapchainImageViews.size(); i++)
    {
        attachments[0] = swapchainImageViews[i];
        attachments[1] = depthImageView;

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = (uint32)(sizeof(attachments) / sizeof(attachments[0]));
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchainImageExtent.width;
        framebufferInfo.height = swapchainImageExtent.height;
        framebufferInfo.layers = 1; //number of layers in image arrays

        if(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainFramebuffers[i])
            != VK_SUCCESS)
        {
            throw std::runtime_error("FATAL_ERROR: Unable to create framebuffer.");
        }
    }
}

void VulkanManager::createCommandPool()
{
    VkCommandPoolCreateInfo  cmdPoolInfo{};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    cmdPoolInfo.flags = 0;

    if(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL ERROR: Unable to create command pool.");
    }
}

void VulkanManager::createDepthResources()
{
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    createImage(swapchainImageExtent.width, 
                swapchainImageExtent.height,
                depthFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                depthImage,
                depthImageMemory);
    
    
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if(vkCreateImageView(device, &viewInfo, nullptr, &depthImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL ERROR: Unable to create an image view for the depth buffer.");
    }
}

void VulkanManager::createTextureImage()
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("textures/will_confia.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    
    //4 bytes per pixel;
    VkDeviceSize imgSize = texWidth * texHeight * 4;

    if(!pixels)
    {
        throw std::runtime_error("FATAL ERROR: unable to load texture image.");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(imgSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, 
                 stagingBufferMemory);

    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, imgSize, 0, &data);
    memcpy(data, pixels, (size_t)(imgSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(texWidth, texHeight,
                VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImg, textureImgMemory);

    transitionImageLayout(textureImg,
                           VK_FORMAT_R8G8B8A8_SRGB, 
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    copyBufferToImage(stagingBuffer, textureImg, (uint32)(texWidth), (uint32)(texHeight));

    transitionImageLayout(textureImg,
                           VK_FORMAT_R8G8B8A8_SRGB, 
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanManager::createTextureImageView()
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = textureImg;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    if(vkCreateImageView(device, &viewInfo, nullptr, &textureImgView) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL ERROR: Unable to create an image view for a texture.");
    }
}

void VulkanManager::createTextureSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if(vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL ERROR: Unable to create texture sampler.");
    }
}

void VulkanManager::createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size(); 


    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, 
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                 stagingBuffer, 
                 stagingBufferMemory);
   
    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)(bufferSize));
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, 
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                 vertexBuffer, 
                 vertexBufferMemory);

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanManager::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(swapchainImages.size());
    uniformBuffersMemory.resize(swapchainImages.size());

    for(size_t i = 0; i < swapchainImages.size(); i++) 
    {
        createBuffer(bufferSize, 
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                    uniformBuffers[i], 
                    uniformBuffersMemory[i]);
    }
}

void VulkanManager::createIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, 
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                 stagingBuffer, 
                 stagingBufferMemory);


    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)(bufferSize));
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, 
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                 indexBuffer, 
                 indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

VkCommandBuffer VulkanManager::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = cmdPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanManager::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);

}

void VulkanManager::createCommandBuffers()
{
    //create command buffers, one for each swapchain image:
    cmdBuffers.resize(swapchainFramebuffers.size());

    VkCommandBufferAllocateInfo cmdBufferAllocInfo{};
    cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAllocInfo.commandPool = cmdPool;
    cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferAllocInfo.commandBufferCount = (uint32)cmdBuffers.size();

    if(vkAllocateCommandBuffers(device, &cmdBufferAllocInfo, cmdBuffers.data()))
    {
        throw std::runtime_error("FATAL ERROR : Unable to allocate command buffers!");
    }

    //record command buffers:
    for(size_t i = 0; i < cmdBuffers.size(); i++)
    {
        VkCommandBufferBeginInfo cmdBufferBeginInfo{};
        cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufferBeginInfo.flags = 0; //Optional
        cmdBufferBeginInfo.pInheritanceInfo = nullptr; //Optional

        if( vkBeginCommandBuffer(cmdBuffers[i], &cmdBufferBeginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("FATAL ERROR: Unable to record command buffer");
        }
        
        VkClearValue clearValues[2] = {};
        clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapchainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapchainImageExtent;
        renderPassInfo.clearValueCount = (uint32)(sizeof(clearValues) / sizeof(clearValues[0]));
        renderPassInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(cmdBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(cmdBuffers[i], 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(cmdBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        //set viewport and scissor dynamically
        VkViewport vp{};
        vp.width = (float) swapchainImageExtent.width;
        vp.height = (float) swapchainImageExtent.height;
        vp.minDepth = 0.0f;
        vp.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.extent.width = (float) swapchainImageExtent.width;
        scissor.extent.height = (float) swapchainImageExtent.height;

        vkCmdSetViewport(cmdBuffers[i], 0, 1, &vp);
        vkCmdSetScissor(cmdBuffers[i], 0, 1, &scissor);

        vkCmdBindDescriptorSets(cmdBuffers[i], 
                                VK_PIPELINE_BIND_POINT_GRAPHICS, 
                                pipelineLayout,
                                0,
                                1,
                                &descriptorSets[i],
                                0,
                                nullptr);

        //draw 3 vertices with 1 instance
        vkCmdDrawIndexed(cmdBuffers[i], (uint32)(indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(cmdBuffers[i]);

        if(vkEndCommandBuffer(cmdBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

void VulkanManager::createSyncPrimitives()
{
    imageAvailableSemaphores.resize(MAX_FRAMES);
    renderFinishedSemaphores.resize(MAX_FRAMES);
    queueSubmitFences.resize(MAX_FRAMES);
    imageInUseFences.resize(swapchainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;


    for(size_t i = 0; i < MAX_FRAMES; i++)
    {
        if(   vkCreateSemaphore(device, 
                                &semaphoreInfo, 
                                nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS 
            || vkCreateSemaphore(device, 
                                &semaphoreInfo, 
                                nullptr, 
                                &renderFinishedSemaphores[i]) != VK_SUCCESS
            || vkCreateFence(device,
                            &fenceInfo,
                            nullptr,
                            &queueSubmitFences[i]) != VK_SUCCESS
            )
        {
            throw std::runtime_error(
            "FATAL ERROR: Unable to create synchronization objects for a frame."
                                    );
        }
    }
}


void VulkanManager::destroySwapchain()
{
    for(VkFramebuffer framebuffer : swapchainFramebuffers)
    {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    
    //no need to destroy command pool before recreating swapchain for this simple application
    vkFreeCommandBuffers(device, cmdPool, (uint32)(cmdBuffers.size()), cmdBuffers.data());

    //pipeline and pipeline layout
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

    //render pass
    vkDestroyRenderPass(device, renderPass, nullptr);

    //imageviews
    for(VkImageView imageView : swapchainImageViews)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);

    for(size_t i = 0; i < swapchainImages.size(); i++)
    {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

}

void VulkanManager::resize()
{
    int width = 0; int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while(width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    destroySwapchain();

    createSwapchain();
    createRenderPass();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();

    imageInUseFences.resize(swapchainImages.size(), VK_NULL_HANDLE);
}

void VulkanManager::updateUniformBuffer(uint32 currImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currTime = std::chrono::high_resolution_clock::now();
    
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f),  
                            time * glm::radians(90.0f), 
                            glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
                           glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.proj = glm::perspective(glm::radians(45.0f), 
                                (swapchainImageExtent.width / (float)(swapchainImageExtent.height)),
                                0.1f,
                                10.0f);

    //flip y
    ubo.proj[1][1] *= -1;

    void* data;
    vkMapMemory(device, uniformBuffersMemory[currImage], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, uniformBuffersMemory[currImage]);


}

void VulkanManager::renderFrame()
{
    vkWaitForFences(device, 1, &queueSubmitFences[currFrame], VK_TRUE, UINT64_MAX);

    uint32 imgIndex;
    //UINT64_MAX disables timeout
    VkResult result = vkAcquireNextImageKHR(device, 
                                            swapchain, 
                                            UINT64_MAX, 
                                            imageAvailableSemaphores[currFrame],
                                            VK_NULL_HANDLE,
                                            &imgIndex);

    if(result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        resize();
        return;
    }
    if((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR))
    {
        throw std::runtime_error("FATAL_ERROR: Unable to acquire swapchain image.");
    }

    //check if a previous frame is using this image
    if(imageInUseFences[imgIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(device, 1, &queueSubmitFences[imgIndex], VK_TRUE, UINT64_MAX);
    }

    //mark image as being used by this frame
    imageInUseFences[imgIndex] = queueSubmitFences[currFrame];

    //update uniform buffers
    updateUniformBuffer(imgIndex);

    //submit command buffer
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currFrame]};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffers[imgIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(device, 1, &queueSubmitFences[currFrame]);

    if(vkQueueSubmit(graphicsQueue, 1, &submitInfo, queueSubmitFences[currFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL ERROR: Unable to submit command buffer");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imgIndex;
    presentInfo.pResults = nullptr; //options
    
    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR) || windowWasResized)
    {
        resize();
    }
    else if(result != VK_SUCCESS)
    {
        throw std::runtime_error("FATAL ERROR: Unable to present swapchain image.");
    }

    currFrame = (currFrame + 1) % MAX_FRAMES;
}

void VulkanManager::shutDown()
{
    destroySwapchain();
    
    //textures
    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroyImageView(device, textureImgView, nullptr);
    vkDestroyImage(device, textureImg, nullptr);
    vkFreeMemory(device, textureImgMemory, nullptr);

    //descriptor set layout
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    
    //index buffer
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    //vertex buffer
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    //semaphores and fences
    for(size_t i = 0; i < MAX_FRAMES; i++)
    {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, queueSubmitFences[i], nullptr);
    }

    //command pool
    vkDestroyCommandPool(device, cmdPool, nullptr);

    //device
    vkDestroyDevice(device, nullptr);

    //surface
    vkDestroySurfaceKHR(instance, surface, nullptr);

    //instance
    vkDestroyInstance(instance, nullptr);
}





















