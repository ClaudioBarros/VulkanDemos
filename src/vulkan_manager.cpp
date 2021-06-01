#include "vulkan_manager.h"
#include <stdexcept>
#include <algorithm>
#include <fstream>

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
    swapchainImageFormat = selectedFormat->format;
    swapchainImageExtent = selectedImageExtent;

    //--------------- IMAGE VIEWS ----------------

    //create image views:
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
    //------------------- RENDER PASS --------------------
    //create render pass:
    {
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

        //Create subpass
        //There will be a single subpass with a color attachment.

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0; //index of our single attachment
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        //there will be two layout transitions:
        //UNDEFINED -> COLOR_ATTACHMENT_OPTIONAL.
        //COLOR_ATTACHMENT_OPTIONAL -> PRESENT_SRC_KHR;
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        //need to wait for the Window System Integration semaphore to signal
        //"Only pipeline stages which depend on COLOR_ATTACHMENT_OUTPUT_BIT will
        // actually wait for the semaphore, so we must also wait for that pipeline stage."
        // src: official vulkan samples.
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        //create the render pass:
        VkRenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = 1; 
        renderPassCreateInfo.pAttachments = &colorAttachment;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpass;
        renderPassCreateInfo.dependencyCount = 1;
        renderPassCreateInfo.pDependencies = &dependency;

        if(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("FATAL_ERROR: Unable to create render pass.");
        }
    }

    //------------------- GRAPHICS PIPELINE ------------------
    //create graphics pipeline:
    {
        //don't need to bind any resources for a simple triangle application
        //since the necessary data will be defined directly in the vertex shader
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        if(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout)
           != VK_SUCCESS)
        {
            throw std::runtime_error("FATAL_ERROR: Unable to create pipeline.");
        }

        VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
        vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        //specify triangle lists as topology to draw geometry
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
        inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        //specify rasterization state
        VkPipelineRasterizationStateCreateInfo rasterCreateInfo{};
        rasterCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
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

        //specify no multisampling
        VkPipelineMultisampleStateCreateInfo multisampleCreateInfo{};
        multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

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
            std::string vertexShaderFilename = "shaders/triangle/triangle_vert.spv";
            std::string fragmentShaderFilename = "shaders/triangle/triangle_frag.spv";

            //start reading at the end of the file to be able to determine file size 
            //spir-v files need to be read in binary mode
            std::ifstream vertShaderFile(vertexShaderFilename, std::ios::ate | std::ios::binary);
            std::ifstream fragShaderFile(vertexShaderFilename, std::ios::ate | std::ios::binary);

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

}

void VulkanManager::shutDown()
{
    for(VkImageView imageView : swapchainImageViews)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}





















