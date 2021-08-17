#include "textured_cube.h"
#include "fstream"

const std::vector<float> vertices = 
{
    -1.0f,-1.0f,-1.0f,  // -X side
    -1.0f,-1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Z side
     1.0f, 1.0f,-1.0f,
     1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Y side
     1.0f,-1.0f,-1.0f,
     1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
     1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,

    -1.0f, 1.0f,-1.0f,  // +Y side
    -1.0f, 1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f, 1.0f,
     1.0f, 1.0f,-1.0f,

     1.0f, 1.0f,-1.0f,  // +X side
     1.0f, 1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f,-1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f, 1.0f, 1.0f,  // +Z side
    -1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
};

const std::vector<float> texCoords =
{
    0.0f, 1.0f,  // -X side
    1.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,  // -Z side
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // -Y side
    1.0f, 1.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // +Y side
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,

    1.0f, 0.0f,  // +X side
    0.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,

    0.0f, 0.0f,  // +Z side
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
}; 

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData) 
{
    std::string output("\n[VULKAN] ");
    output.append(pCallbackData->pMessage);
    LOGI(output);
    
    return VK_FALSE;
}

VkSurfaceFormatKHR pickSurfaceFormat(const VkSurfaceFormatKHR *surfaceFormats, uint32_t count) {
    // Prefer non-SRGB formats...
    for (uint32_t i = 0; i < count; i++) {
        const VkFormat format = surfaceFormats[i].format;

        if (format == VK_FORMAT_R8G8B8A8_UNORM || format == VK_FORMAT_B8G8R8A8_UNORM ||
            format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 || format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 ||
            format == VK_FORMAT_R16G16B16A16_SFLOAT) {
            return surfaceFormats[i];
        }
    }

    LOGI("Can't find our preferred formats.Falling back to first exposed format. Rendering may be incorrect.\n");

    assert(count >= 1);
    return surfaceFormats[0];
}


void Demo::startUp()
{
    isInitialized = false;
    isMinimized = false;
    isPrepared = false;

    this->width = 1280;
    this->height = 720;
    
    //geometry data
    vertexData = vertices;
    uvData = texCoords; 

    //load texture files
    textures.resize(1);
    vulkanTextures.resize(1);
    textures[0].load(std::string("textures/wooden_crate.png"));

    //--------- mouse ----------
    confineMouseCursorToWindow();
    centerMouseCursor();
    ShowCursor(FALSE);
    mouseSensitivity = 0.6f;
    //======== window ===========
    window.init(this, L"Vulkan Demo - Textured Cube", this->width, this->height);
    window.updateScreenCoordinates();

    //====== Vulkan configuration ===== 
    VulkanConfig vulkanConfig{};
    vulkanConfig.appName = "Textured Cube";

    // --- validation layers ---
    vulkanConfig.enableValidationLayers = true;
    vulkanConfig.validationLayers.push_back("VK_LAYER_KHRONOS_validation");

    //--- extensions ---
    vulkanConfig.instanceExtensions = {
    "VK_KHR_device_group_creation",
	"VK_KHR_display",
	"VK_KHR_external_fence_capabilities",
	"VK_KHR_external_memory_capabilities",
	"VK_KHR_external_semaphore_capabilities",
	"VK_KHR_get_display_properties2",
	"VK_KHR_get_physical_device_properties2",
	"VK_KHR_get_surface_capabilities2",
	"VK_KHR_surface",
	"VK_KHR_surface_protected_capabilities",
	"VK_KHR_win32_surface",
	"VK_EXT_debug_report",
	"VK_EXT_debug_utils",
    "VK_EXT_swapchain_colorspace",
	"VK_NV_external_memory_capabilities",
    };
    
    vulkanConfig.deviceExtensions = {"VK_KHR_swapchain"};
    
    // --- physical device features to enable ------
    vulkanConfig.physDeviceFeaturesToEnable.samplerAnisotropy = VK_TRUE;

    //--- formats ---
    vulkanConfig.preferredDepthFormat = VK_FORMAT_D16_UNORM;
    vulkanConfig.preferredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    vulkanConfig.preferredSurfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
    vulkanConfig.preferredSurfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    vulkanConfig.texFormat = VK_FORMAT_R8G8B8A8_UNORM;

    vulkanConfig.ptrDebugMessenger = debugCallback;
    
    vulkanManager.isMinimized = &isMinimized;
    vulkanManager.startUp(&window, vulkanConfig, &this->width, &this->height);
    //======== camera ===========
    movementSpeed = 20.0f;
    glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 7.0f);

    float yaw = -90.0f;  
    float pitch = 0.0f;
    camera.init(cameraPos, yaw, pitch);
    
    float yFov = glm::radians(45.0f);
    float aspect = (float)(this->width) / (float)(this->height); 
    float n = 0.1f;
    float f = 100.0f;

    modelMatrix = glm::mat4(1.0f);
    
    viewMatrix = camera.getViewMatrix();

    projMatrix = glm::perspective(yFov, aspect, n, f),
    projMatrix[1][1] *= -1;

    isInitialized = true;
}

void Demo::shutDown()
{
    isPrepared = false;
    vkDeviceWaitIdle(vulkanManager.logicalDevice.device);

    for(size_t i = 0; i < vulkanTextures.size(); i++)
    {
        vulkanManager.freeVulkanTexture(vulkanTextures[i]);
    } 
    
    vkDestroyDescriptorPool(vulkanManager.logicalDevice.device, 
                            descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(vulkanManager.logicalDevice.device, 
                                 descriptorSetLayout, 
                                 nullptr);
    
    //if the window is minized, resize() has already done some cleanup.
    if(!isMinimized)
    {
        vulkanManager.shutDown(); 
    }
}

void Demo::prepare()
{
    vulkanManager.initCmdPool();

    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandPool = vulkanManager.cmdPool;
    cmdAllocInfo.commandBufferCount = 1;
    
    VK_CHECK(vkAllocateCommandBuffers(vulkanManager.logicalDevice.device,
                                      &cmdAllocInfo, 
                                      &vulkanManager.cmdBuffer));

    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(vulkanManager.cmdBuffer, &cmdBeginInfo));

    vulkanManager.swapchain.queryInfoAndChooseSettings(vulkanManager.physicalDevice.device,
                                                       vulkanManager.logicalDevice.device,
                                                       vulkanManager.surface,
                                                       vulkanManager.config.preferredSurfaceFormat,
                                                       vulkanManager.config.preferredPresentMode,
                                                       &this->width, &this->height);
    
    if(this->width == 0 || 
       this->height == 0)
    {
        isMinimized = true;
    }
    else
    {
        isMinimized = false;
    }
    
    if(isMinimized)
    {
        isPrepared = false;
        return;
    }   

    vulkanManager.swapchain.createSwapchainAndImageResources(vulkanManager.surface,
                                                             vulkanManager.logicalDevice.device);

    vulkanManager.initDepthImage(vulkanManager.config.preferredDepthFormat,
                                 this->width, this->height); 

    initStagingTexture();
    initTextures();
    initCubeDataBuffers();
    initDescriptorLayout();
    initRenderPass();
    initPipeline();

    for(uint32 i = 0; i < vulkanManager.swapchain.imageCount; i++)
    {
        VK_CHECK(vkAllocateCommandBuffers(vulkanManager.logicalDevice.device,
                                          &cmdAllocInfo,
                                          &vulkanManager.swapchain.imageResources[i].cmd));
    }

    if(vulkanManager.physicalDevice.separatePresentQueue)
    {
        VkCommandPoolCreateInfo presentCmdPoolInfo{};
        presentCmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        presentCmdPoolInfo.pNext = nullptr;
        presentCmdPoolInfo.queueFamilyIndex = vulkanManager.physicalDevice.presentQueueFamilyIndex;
        
        VK_CHECK(vkCreateCommandPool(vulkanManager.logicalDevice.device, 
                                     &presentCmdPoolInfo,
                                     nullptr,
                                     &vulkanManager.presentCmdPool));
        
        VkCommandBufferAllocateInfo presentCmdAllocInfo{};
        presentCmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        presentCmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        presentCmdAllocInfo.commandPool = vulkanManager.presentCmdPool;
        presentCmdAllocInfo.commandBufferCount = 1;

        for(uint32 i = 0; i < vulkanManager.swapchain.imageCount; i++)
        {
            VK_CHECK(vkAllocateCommandBuffers(vulkanManager.logicalDevice.device, 
                                              &presentCmdAllocInfo,
                                              &vulkanManager.swapchain.imageResources[i].graphicsToPresentCmd));
            setupImageOwnership(i);
        }
    }
    
    initDescriptorPool();
    initDescriptorSet();
    initFramebuffers(); 
    
    for(uint32 i = 0; i < vulkanManager.swapchain.imageCount; i++)
    {
        currBufferIndex = i;
        recordDrawCommands(vulkanManager.swapchain.imageResources[i].cmd);
    } 
    
    //flush pipeline commands before beginning the render loop 
    flushInitCmd();

    vulkanManager.freeVulkanTexture(stagingTexture);
    
    currBufferIndex = 0;
    isPrepared = true;
}

void Demo::initStagingTexture()
{
    stagingTexture = {};
    stagingTexture.width = textures[0].width;
    stagingTexture.height = textures[0].height;
    VkDeviceSize imgSize = stagingTexture.width* stagingTexture.height * 4;
    
    vulkanManager.initBuffer(imgSize,
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             stagingTexture.buffer, 
                             stagingTexture.mem); 
}

void Demo::initTextures()
{

    assert((textures[0].pixels != nullptr) && (textures[0].height > 0) && (textures[0].width > 0));
    assert((stagingTexture.buffer != VK_NULL_HANDLE) && (stagingTexture.mem != nullptr));

    //4 bytes per pixel;
    uint32 texWidth = textures[0].width;
    uint32 texHeight = textures[0].height;
    uint8 *texPixels = textures[0].pixels;
    VkDeviceSize imgSize = texWidth * texHeight * 4;

    //copy texture data to staging buffer
    void *data;
    vkMapMemory(vulkanManager.logicalDevice.device, stagingTexture.mem, 0, imgSize, 0, &data);
    memcpy(data, texPixels, (size_t)(imgSize));
    vkUnmapMemory(vulkanManager.logicalDevice.device, stagingTexture.mem);

    //init texture image 

    VkMemoryPropertyFlags requiredImageProperties = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    vulkanManager.initImage(texWidth, texHeight,
                            vulkanManager.config.texFormat, VK_IMAGE_TILING_OPTIMAL,
                            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED,
                            vulkanTextures[0].image, vulkanTextures[0].mem);

    vulkanTextures[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vulkanManager.setImageLayout(vulkanTextures[0].image,
                                 VK_IMAGE_ASPECT_COLOR_BIT,
                                 VK_IMAGE_LAYOUT_PREINITIALIZED,
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

    vkCmdCopyBufferToImage(vulkanManager.cmdBuffer, 
                           stagingTexture.buffer, 
                           vulkanTextures[0].image, 
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &copyRegion);

    vulkanManager.setImageLayout(vulkanTextures[0].image,
                                 VK_IMAGE_ASPECT_COLOR_BIT,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 vulkanTextures[0].imageLayout, 
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
    samplerInfo.maxAnisotropy = vulkanManager.physicalDevice.properties.limits.maxSamplerAnisotropy;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    
    VK_CHECK(vkCreateSampler(vulkanManager.logicalDevice.device, 
                             &samplerInfo, nullptr, &vulkanTextures[0].sampler));

    //--- image view ----
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = vulkanTextures[0].image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = vulkanManager.config.texFormat;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    VK_CHECK(vkCreateImageView(vulkanManager.logicalDevice.device, 
                               &viewInfo, nullptr, &vulkanTextures[0].view));

}

void Demo::initCubeDataBuffers()
{
    VS_UBO data{};

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    viewMatrix = camera.getViewMatrix();
    data.mvp = projMatrix * viewMatrix * modelMatrix;

    //vulkan expects the y coord to be flipped
    //data.mvp[1][1] *= -1;

    for(size_t i = 0; i < (12 * 3); i++)
    {
        assert((i * 3 + 2) < vertexData.size());
        data.pos[i] = glm::vec4(vertexData[i * 3],
                                vertexData[i * 3 + 1],
                                vertexData[i * 3 + 2],
                                1.0f);

        data.attr[i] = glm::vec4(texCoords[i * 2],
                                 texCoords[i * 2 + 1],
                                 0.0f,
                                 0.0f);
    }

    for(size_t i = 0; i < vulkanManager.swapchain.imageResources.size(); i++) 
    {
        vulkanManager.initBuffer(sizeof(data), 
                                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 vulkanManager.swapchain.imageResources[i].uniformBuffer, 
                                 vulkanManager.swapchain.imageResources[i].uniformMemory);

        VK_CHECK(vkMapMemory(vulkanManager.logicalDevice.device,
                             vulkanManager.swapchain.imageResources[i].uniformMemory,
                             0, VK_WHOLE_SIZE, 0, 
                             &vulkanManager.swapchain.imageResources[i].uniformMemoryPtr));

        memcpy(vulkanManager.swapchain.imageResources[i].uniformMemoryPtr, &data, sizeof(data));
    }
}

void Demo::initDescriptorLayout()
{
    VkDescriptorSetLayoutBinding layoutBindings[2] = {};

    //mvp + pos + tex_coords
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBindings[0].pImmutableSamplers = nullptr;

    //sampler
    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[1].descriptorCount = (uint32)textures.size();
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = layoutBindings;

    VK_CHECK(vkCreateDescriptorSetLayout(vulkanManager.logicalDevice.device,
                                         &layoutInfo,
                                         nullptr,
                                         &descriptorSetLayout));
}

void Demo::initRenderPass()
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
    colorAttachment.format = vulkanManager.swapchain.surfaceFormat.format; //must match swapchain format
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
    depthAttachment.format = vulkanManager.config.preferredDepthFormat;
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

    VK_CHECK(vkCreateRenderPass(vulkanManager.logicalDevice.device, 
                                &renderPassCreateInfo, 
                                nullptr, 
                                &vulkanManager.renderPass));
}

void Demo::initPipeline()
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    
    VK_CHECK(vkCreatePipelineLayout(vulkanManager.logicalDevice.device, 
                                    &pipelineLayoutInfo, 
                                    nullptr, 
                                    &vulkanManager.pipelineLayout));
    

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    //specify triangle lists as topology to draw geometry
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    //specify rasterization state
    VkPipelineRasterizationStateCreateInfo rasterInfo{};
    rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterInfo.depthClampEnable = VK_FALSE;
    rasterInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterInfo.depthBiasEnable = VK_FALSE;
    rasterInfo.lineWidth = 1.0f;

    //attachment will write to all color channels
    //no blending enabled
    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask =    VK_COLOR_COMPONENT_R_BIT | 
                                        VK_COLOR_COMPONENT_G_BIT |
                                        VK_COLOR_COMPONENT_B_BIT |
                                        VK_COLOR_COMPONENT_A_BIT;
    blendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo blendStateInfo{};
    blendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendStateInfo.attachmentCount = 1;
    blendStateInfo.pAttachments = &blendAttachment;

    //specify that one viewport and one scissor box will be used
    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.scissorCount = 1;

    //disable depth and stencil testing
    VkPipelineDepthStencilStateCreateInfo depthInfo{};
    depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthInfo.depthTestEnable = VK_TRUE;
    depthInfo.depthWriteEnable = VK_TRUE;
    depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthInfo.depthBoundsTestEnable = VK_FALSE;
    depthInfo.minDepthBounds = 0.0f;
    depthInfo.maxDepthBounds = 1.0f;
    depthInfo.stencilTestEnable = VK_FALSE;
    depthInfo.back.failOp = VK_STENCIL_OP_KEEP;
    depthInfo.back.passOp = VK_STENCIL_OP_KEEP;
    depthInfo.front = depthInfo.back;

    //specify no multisampling
    VkPipelineMultisampleStateCreateInfo multisampleInfo{};
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    //Specify that the viewport and scissor states will be dynamic.
    std::vector<VkDynamicState> dynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicStatesCreateInfo{};
    dynamicStatesCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStatesCreateInfo.pDynamicStates = dynamicStates.data();
    dynamicStatesCreateInfo.dynamicStateCount = (uint32)(dynamicStates.size());

    //Load shader modules
    std::vector<char> vsBuffer;
    std::vector<char> fsBuffer;
    
    std::string vsFilename = "shaders/textured_cube/textured_cube_vert.spv";
    std::string fsFilename = "shaders/textured_cube/textured_cube_frag.spv";
    
    loadShaderModule(vsFilename, vsBuffer);
    loadShaderModule(fsFilename, fsBuffer);

    VkShaderModuleCreateInfo vertShaderCreateInfo{};
    vertShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertShaderCreateInfo.codeSize = vsBuffer.size();
    vertShaderCreateInfo.pCode = reinterpret_cast<const uint32*>(vsBuffer.data());

    VkShaderModuleCreateInfo fragShaderCreateInfo{};
    fragShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragShaderCreateInfo.codeSize = fsBuffer.size();
    fragShaderCreateInfo.pCode = reinterpret_cast<const uint32*>(fsBuffer.data());
    
    //create shader modules 
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    
    VK_CHECK(vkCreateShaderModule(vulkanManager.logicalDevice.device, 
                                  &vertShaderCreateInfo, 
                                  nullptr, 
                                  &vertShaderModule));
    VK_CHECK(vkCreateShaderModule(vulkanManager.logicalDevice.device, 
                                  &fragShaderCreateInfo, 
                                  nullptr, 
                                  &fragShaderModule));

    VkPipelineShaderStageCreateInfo vsInfo{};
    vsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vsInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vsInfo.module = vertShaderModule;
    vsInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fsInfo{};
    fsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fsInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fsInfo.module = fragShaderModule;
    fsInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vsInfo, fsInfo};
    
    //init pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheInfo{};
    pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK(vkCreatePipelineCache(vulkanManager.logicalDevice.device,
                                   &pipelineCacheInfo,
                                   nullptr,
                                   &vulkanManager.pipelineCache));

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2; //2 -> vertex and shader stages
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pRasterizationState = &rasterInfo;
    pipelineInfo.pColorBlendState = &blendStateInfo;
    pipelineInfo.pMultisampleState = &multisampleInfo;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pDepthStencilState = &depthInfo;
    pipelineInfo.pDynamicState = &dynamicStatesCreateInfo;
    pipelineInfo.renderPass = vulkanManager.renderPass;
    pipelineInfo.layout = vulkanManager.pipelineLayout;

    VK_CHECK(vkCreateGraphicsPipelines(vulkanManager.logicalDevice.device, 
                                       vulkanManager.pipelineCache, 
                                       1, 
                                       &pipelineInfo, 
                                       nullptr, 
                                       &vulkanManager.pipeline));

    //shader modules are safe to destroy after the graphics pipeline is created
    vkDestroyShaderModule(vulkanManager.logicalDevice.device, fragShaderModule, nullptr);
    vkDestroyShaderModule(vulkanManager.logicalDevice.device, vertShaderModule, nullptr);
}

void Demo::setupImageOwnership(int i)
{
    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    cmdBeginInfo.pInheritanceInfo = nullptr;
    
    VK_CHECK(vkBeginCommandBuffer(vulkanManager.swapchain.imageResources[i].graphicsToPresentCmd,
                                  &cmdBeginInfo));
    
    VkImageMemoryBarrier imageOwnershipBarrier{};
    imageOwnershipBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageOwnershipBarrier.srcAccessMask = 0;
    imageOwnershipBarrier.dstAccessMask = 0;
    imageOwnershipBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    imageOwnershipBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    imageOwnershipBarrier.srcQueueFamilyIndex = vulkanManager.physicalDevice.graphicsQueueFamilyIndex;
    imageOwnershipBarrier.dstQueueFamilyIndex = vulkanManager.physicalDevice.presentQueueFamilyIndex;
    imageOwnershipBarrier.image = vulkanManager.swapchain.imageResources[i].image;
    imageOwnershipBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageOwnershipBarrier.subresourceRange.baseMipLevel = 0;
    imageOwnershipBarrier.subresourceRange.levelCount = 1;
    imageOwnershipBarrier.subresourceRange.baseArrayLayer = 0;
    imageOwnershipBarrier.subresourceRange.layerCount = 1;
    
    vkCmdPipelineBarrier(vulkanManager.swapchain.imageResources[i].graphicsToPresentCmd,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &imageOwnershipBarrier);
    
    VK_CHECK(vkEndCommandBuffer(vulkanManager.swapchain.imageResources[i].graphicsToPresentCmd));
}

void Demo::initDescriptorPool()
{
    VkDescriptorPoolSize poolSizes[2] = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = vulkanManager.swapchain.imageCount;
    
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = vulkanManager.swapchain.imageCount * 
                                   (uint32)(textures.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = vulkanManager.swapchain.imageCount;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    
    VK_CHECK(vkCreateDescriptorPool(vulkanManager.logicalDevice.device, 
                                    &poolInfo, 
                                    nullptr,
                                    &descriptorPool));
}

void Demo::initDescriptorSet()
{
    std::vector<VkDescriptorImageInfo> texDescriptorInfos(vulkanTextures.size(), VkDescriptorImageInfo{});

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1; 
    allocInfo.pSetLayouts = &descriptorSetLayout; 
    
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(VS_UBO);

    for(uint32 i = 0; i < textures.size();i++)
    {
        texDescriptorInfos[i].sampler = vulkanTextures[i].sampler;
        texDescriptorInfos[i].imageView = vulkanTextures[i].view;
        texDescriptorInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkWriteDescriptorSet writeDescriptorSets[2] = {};

    writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[0].dstBinding = 0;
    writeDescriptorSets[0].descriptorCount = 1;
    writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSets[0].pBufferInfo = &bufferInfo;
    
    writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[1].dstBinding = 1;
    writeDescriptorSets[1].descriptorCount = (uint32)(vulkanTextures.size());
    writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSets[1].pImageInfo = texDescriptorInfos.data();
    
    for(uint32 i = 0; i < vulkanManager.swapchain.imageCount; i++)
    {
        VK_CHECK(vkAllocateDescriptorSets(vulkanManager.logicalDevice.device,
                                          &allocInfo,
                                          &vulkanManager.swapchain.imageResources[i].descriptorSet));

        bufferInfo.buffer = vulkanManager.swapchain.imageResources[i].uniformBuffer;
        writeDescriptorSets[0].dstSet = vulkanManager.swapchain.imageResources[i].descriptorSet;
        writeDescriptorSets[1].dstSet = vulkanManager.swapchain.imageResources[i].descriptorSet;

        vkUpdateDescriptorSets(vulkanManager.logicalDevice.device, 2, writeDescriptorSets, 0, nullptr);
    }
}

void Demo::initFramebuffers()
{
    VkImageView attachments[2] = {};
    attachments[1] = vulkanManager.depth.view;

    VkFramebufferCreateInfo fbInfo{}; 
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = vulkanManager.renderPass;
    fbInfo.attachmentCount = 2;
    fbInfo.pAttachments = attachments;
    fbInfo.width = this->width;
    fbInfo.height = this->height;
    fbInfo.layers = 1;
    
    for(uint32 i = 0; i < vulkanManager.swapchain.imageCount; i++)
    {
        attachments[0] = vulkanManager.swapchain.imageResources[i].view;
        
        VK_CHECK(vkCreateFramebuffer(vulkanManager.logicalDevice.device, 
                            &fbInfo, 
                            nullptr,
                            &vulkanManager.swapchain.imageResources[i].framebuffer));
    }
}

void Demo::recordDrawCommands(VkCommandBuffer cmdBuffer)
{
    VkCommandBufferBeginInfo cmdBufferInfo{};
    cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VkClearValue clearValues[2] = {};
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo rpBeginInfo{};
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = vulkanManager.renderPass;
    rpBeginInfo.framebuffer = vulkanManager.swapchain.imageResources[currBufferIndex].framebuffer;
    rpBeginInfo.renderArea.offset = {0, 0};
    rpBeginInfo.renderArea.extent.width = this->width;
    rpBeginInfo.renderArea.extent.height = this->height;
    rpBeginInfo.clearValueCount = (uint32)(sizeof(clearValues) / sizeof(clearValues[0]));
    rpBeginInfo.pClearValues = clearValues;

    VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBufferInfo));
    
    vkCmdBeginRenderPass(cmdBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmdBuffer, 
                      VK_PIPELINE_BIND_POINT_GRAPHICS, 
                      vulkanManager.pipeline);
    
    vkCmdBindDescriptorSets(cmdBuffer, 
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            vulkanManager.pipelineLayout,
                            0, 
                            1,
                            &vulkanManager.swapchain.imageResources[currBufferIndex].descriptorSet,
                            0,
                            nullptr);
    
    //viewport
    VkViewport vp{};
    vp.width = (float) this->width;
    vp.height = (float) this->height;
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuffer, 0, 1, &vp);

    //scissor 
    VkRect2D scissor{};
    scissor.extent.width = this->width;
    scissor.extent.height = this->height;
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
    
    vkCmdDraw(cmdBuffer, (uint32)(vertexData.size()/3), 
              1, 0, 0);

    //NOTE(): Ending the render pass changes the image's layout from
    //        COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR
    vkCmdEndRenderPass(cmdBuffer);

    if(vulkanManager.physicalDevice.separatePresentQueue)
    {
        //Transfer ownership from graphics queue family to present queue family.
        //No need to transfer it back to graphics queue family.
        //NOTE(): maybe do this with semaphores instead.
        
        VkImageMemoryBarrier imageOwnershipBarrier{};
        imageOwnershipBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageOwnershipBarrier.srcAccessMask = 0;
        imageOwnershipBarrier.dstAccessMask = 0;
        imageOwnershipBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        imageOwnershipBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        imageOwnershipBarrier.srcQueueFamilyIndex = vulkanManager.physicalDevice.graphicsQueueFamilyIndex;
        imageOwnershipBarrier.dstQueueFamilyIndex = vulkanManager.physicalDevice.presentQueueFamilyIndex ;
        imageOwnershipBarrier.image = vulkanManager.swapchain.imageResources[currBufferIndex].image;
        imageOwnershipBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageOwnershipBarrier.subresourceRange.baseMipLevel = 0;
        imageOwnershipBarrier.subresourceRange.levelCount = 1;
        imageOwnershipBarrier.subresourceRange.baseArrayLayer = 0;
        imageOwnershipBarrier.subresourceRange.layerCount = 1;
        
        vkCmdPipelineBarrier(cmdBuffer, 
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &imageOwnershipBarrier);
    }
    
    VK_CHECK(vkEndCommandBuffer(cmdBuffer));
}

void Demo::flushInitCmd()
{
    //NOTE(): This function could get called twice if the texture uses a staging buffer.
    //        The second call should be ignored

    if(vulkanManager.cmdBuffer == VK_NULL_HANDLE) return;
    
    VK_CHECK(vkEndCommandBuffer(vulkanManager.cmdBuffer));
    
    VkFence fence;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO; 

    VK_CHECK(vkCreateFence(vulkanManager.logicalDevice.device, &fenceInfo, nullptr, &fence));

    VkCommandBuffer cmdBufs[] = {vulkanManager.cmdBuffer};
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = cmdBufs;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;

    VK_CHECK(vkQueueSubmit(vulkanManager.logicalDevice.graphicsQueue, 1, &submitInfo, fence));
    
    VK_CHECK(vkWaitForFences(vulkanManager.logicalDevice.device, 1, &fence, VK_TRUE, UINT64_MAX));
    
    vkFreeCommandBuffers(vulkanManager.logicalDevice.device, vulkanManager.cmdPool, 1, cmdBufs);
    
    vkDestroyFence(vulkanManager.logicalDevice.device, fence, nullptr);

    vulkanManager.cmdBuffer = VK_NULL_HANDLE;
}

void Demo::resize()
{
    if(!isPrepared)
    {
        if(isMinimized)
        {
            prepare();
        }
        return;
    }
    
    //NOTE: To properly resize the window, the swapchain needs to be recreated,
    //the command buffers need to be re-recorded, etc.

    isPrepared = false;
    vkDeviceWaitIdle(vulkanManager.logicalDevice.device);

    //destroy necessary vulkan objects
    for(size_t i = 0; i < vulkanTextures.size(); i++)
    {
        vulkanManager.freeVulkanTexture(vulkanTextures[i]);
    } 

    vkDestroyDescriptorPool(vulkanManager.logicalDevice.device, 
                            descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(vulkanManager.logicalDevice.device, 
                                 descriptorSetLayout, nullptr);
    
    vulkanManager.prepareForResize();
    //prepare() will recreate them
    prepare();
}

void Demo::updateDataBuffer()
{
    viewMatrix = camera.getViewMatrix();
    glm::mat4 mvp = projMatrix * viewMatrix * modelMatrix;
 
    memcpy(vulkanManager.swapchain.imageResources[currBufferIndex].uniformMemoryPtr,
           (const void *)&mvp[0][0], sizeof(mvp));
}

void Demo::updateAndRender()
{
    if(!isPrepared) return;

    //Make sure that at most MAX_FRAMES rendering are happening at the same time.
    vkWaitForFences(vulkanManager.logicalDevice.device, 1, 
                    &vulkanManager.fences[frameIndex], VK_TRUE, UINT64_MAX);
    vkResetFences(vulkanManager.logicalDevice.device, 1, 
                  &vulkanManager.fences[frameIndex]);
    
    VkResult res;
    do
    {
        //get the index of the next available swapchain image:
        res = 
        vkAcquireNextImageKHR(vulkanManager.logicalDevice.device, 
                              vulkanManager.swapchain.swapchain,
                              UINT64_MAX,
                              vulkanManager.imageAcquiredSemaphores[frameIndex], 
                              VK_NULL_HANDLE, 
                              &currBufferIndex);
        
        if (res == VK_ERROR_OUT_OF_DATE_KHR)
        {
            //swapchain is out of date (window resized, etc) and must be recreated.
            resize();
        }
        
        else if (res == VK_SUBOPTIMAL_KHR)
        {
            //swapchain is not optimal but the image will be correctly presented.
            break;
        }
        else if (res == VK_ERROR_SURFACE_LOST_KHR)
        {
            vkDestroySurfaceKHR(vulkanManager.instance, vulkanManager.surface, nullptr);
            vulkanManager.initSurface(&window);
            resize();
        }
        else
        {
            VK_CHECK(res);
        }
    } while (res != VK_SUCCESS);

    updateDataBuffer();  //TODO

    VkPipelineStageFlags pipelineStageFlags{}; 
    pipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.pWaitDstStageMask = &pipelineStageFlags;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &vulkanManager.imageAcquiredSemaphores[frameIndex];
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkanManager.swapchain.imageResources[currBufferIndex].cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &vulkanManager.drawCompleteSemaphores[frameIndex];

    VK_CHECK(vkQueueSubmit(vulkanManager.logicalDevice.graphicsQueue, 
                           1, &submitInfo, 
                           vulkanManager.fences[frameIndex]));

    if(vulkanManager.physicalDevice.separatePresentQueue)
    {
        //if separate queues are being used, change image ownership to the present queue
        //before presenting, waiting for the draw complete semaphore and signaling the 
        //ownership released semaphore when finished

        VkFence nullFence = VK_NULL_HANDLE; 
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &vulkanManager.drawCompleteSemaphores[frameIndex];
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vulkanManager.swapchain.imageResources[currBufferIndex].graphicsToPresentCmd;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &vulkanManager.imageOwnershipSemaphores[frameIndex];
        
        
        VK_CHECK(vkQueueSubmit(vulkanManager.logicalDevice.presentQueue, 
                               1, &submitInfo, 
                               nullFence));
    }

    // if we are using separate queues we have to wait for image ownership
    // otherwise wait for draw complete    
    
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;

    if(vulkanManager.physicalDevice.separatePresentQueue)
    {
        presentInfo.pWaitSemaphores = &vulkanManager.imageOwnershipSemaphores[frameIndex];
    }
    else
    {
        presentInfo.pWaitSemaphores = &vulkanManager.drawCompleteSemaphores[frameIndex];
    }

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vulkanManager.swapchain.swapchain;
    presentInfo.pImageIndices = &currBufferIndex;
    
    res = vkQueuePresentKHR(vulkanManager.logicalDevice.presentQueue, &presentInfo);
    frameIndex += 1;
    frameIndex %= MAX_FRAMES;

    if (res == VK_ERROR_OUT_OF_DATE_KHR)
    {
        //swapchain is out of date (window resized, etc) and must be recreated.
        resize();
    }
    
    else if (res == VK_SUBOPTIMAL_KHR)
    {
        //swapchain is not optimal but the image will be correctly presented.
    }
    else if (res == VK_ERROR_SURFACE_LOST_KHR)
    {
        vkDestroySurfaceKHR(vulkanManager.instance, vulkanManager.surface, nullptr);
        vulkanManager.initSurface(&window);
        resize();
    }
    else
    {
        VK_CHECK(res);
    }
}

//================== Texture =========================

void Texture::load(std::string filepath)
{
    int texWidth, texHeight, texChannels;
    pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    assert(texWidth > 0);
    assert(texHeight > 0);
    assert(texChannels > 0);

    width = (uint32)(texWidth);
    height = (uint32)(texHeight);
    channels = (uint32)(texChannels);
    filepath = filepath;

}

void Texture::free()
{
    width = 0;
    height = 0;
    channels = 0;
    filepath = "";
    stbi_image_free(pixels);
}

//==================== Shaders =============================
void loadShaderModule(std::string &filename, std::vector<char> &buffer)
{
    //start reading at the end of the file to be able to determine file size 
    //spir-v files need to be read in binary mode
    std::ifstream shaderFile(filename, std::ios::ate | std::ios::binary);

    if(!shaderFile.is_open())
    {
        LOGE_EXIT("Unable to open vertex shader file.");
    }

    size_t filesize = (size_t)shaderFile.tellg();
    buffer.resize(filesize);
    
    shaderFile.seekg(0);

    shaderFile.read(buffer.data(), filesize);

    shaderFile.close();
}

//====================== Input ============================

void Demo::processKeyboardInput(uint64 *pressedKey)
{
    switch(*pressedKey)
    {
        case 0x57: //W key
        {
            //convert frame time to seconds, movementSpeed is expressed in units/sec
            camera.moveForward(movementSpeed, lastFrameTime);
        }break;

        case 0x41: //A key
        {
            camera.moveLeft(movementSpeed, lastFrameTime);
        }break;

        case 0x53: //S key
        {
            camera.moveBackwards(movementSpeed, lastFrameTime);
        }break;

        case 0x44: //D key
        {
            camera.moveRight(movementSpeed, lastFrameTime);
        }break;
        
        default:
        {
        }break; 
    }
}

void Demo::confineMouseCursorToWindow()
{
    ClipCursor(&window.fullscreenCoords);
}

void Demo::centerMouseCursor()
{
    int32 screenCenterX = (int32)((window.fullscreenCoords.right + window.fullscreenCoords.left) / 2);
    int32 screenCenterY = (int32)((window.fullscreenCoords.bottom + window.fullscreenCoords.top)/ 2);
    SetCursorPos(screenCenterX, screenCenterY);
}

void Demo::processMouseInput(float xPos, float yPos)
{
    if(firstMouseInput)
    {
        mouseLastX = xPos;
        mouseLastY = yPos;
        firstMouseInput = false;
    }
    
    float xOffset = xPos - mouseLastX;
    float yOffset = mouseLastY - yPos; //y-coordinates are bottom-up 
    
    mouseLastX = xPos;
    mouseLastY = yPos;

    if(xOffset == 0.0f && yOffset == 0.0f) return;

    camera.rotate(xOffset, yOffset, mouseSensitivity);
    
}

//=================== Windows WndProc Callback ===========================
LRESULT CALLBACK Win32Window::WndProc(UINT   uMsg,
                                      WPARAM wParam,
                                      LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_CLOSE:
        {
            DestroyWindow(this->handle);
            return 0;
        } 
        
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        
        case WM_PAINT:
        {
            if(demo->isInitialized) demo->updateAndRender();
            return 0;
        } 
        
        case WM_GETMINMAXINFO:
        {
            ((MINMAXINFO *)lParam)->ptMinTrackSize = POINT{(LONG)(minX), (LONG)(minY)};
            return 0;
        }
        
        case WM_ERASEBKGND:
        {
            return 1;
        }

        case WM_SIZE:
        {
            if(!demo->isInitialized) return 0;

            //Resize to the new window size, expect when it was minimized.
            //Vulkan doesn't support images or swapchains with width and height = 0
            if(wParam != SIZE_MINIMIZED)
            {
                //The low-order word of lParam specifies the new width of the client area.
                //The high-order word of lParam specifies the new height of the client area.
                demo->width = lParam & 0xFFFF;
                demo->height = (lParam & 0xFFFF0000) >> 16; 
                demo->resize();
            }
            
            updateScreenCoordinates();
            
            return 0;
        } 

        case WM_KEYDOWN:
        {
            demo->processKeyboardInput(&wParam);
            return 0;
        }
        
        case WM_MOUSEMOVE:
        {
            //The low-order word of lParam contain the x coordinate of the mouse
            //The high-order word of lParam specifies the y coordinate of the mouse 
            float xPos = float(int(lParam & 0xFFFF));
            float yPos = float(int((lParam & 0xFFFF0000) >> 16));

            demo->processMouseInput(xPos, yPos);
            
            return 0;
        }

        default:
        {    
        }break;
    }
    
    return DefWindowProcA(handle, uMsg, wParam, lParam);
}

// ======================= Main =====================


int WINAPI WinMain(HINSTANCE hInstance, 
                   HINSTANCE hPrevInstance, 
                   LPSTR pCmdLine, 
                   int nCmdShow)
{
    //setup logger 
    auto msvcSink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("log/debug_output.txt", false);

    std::vector<spdlog::sink_ptr> sinks{msvcSink, fileSink};
    auto _logger = std::make_shared<spdlog::logger>("_logger", begin(sinks), end(sinks));
    spdlog::register_logger(_logger);

    //-----------

    LARGE_INTEGER perfCounterFrequency{};
    QueryPerformanceFrequency(&perfCounterFrequency);

    Demo demo;     
    demo.startUp();
    
    demo.prepare();

    MSG msg{};

    bool isRunning = true;
    while(isRunning)
    {
        LARGE_INTEGER beginPerfCount{};
        QueryPerformanceCounter(&beginPerfCount);

        if(demo.isPaused)
        {
            BOOL succ = WaitMessage();
            if(!succ)
            {
                LOGE_EXIT("WaitMessage() failed on paused demo.");
            }
        }
    
        PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
        if(msg.message == WM_QUIT)
        {
            isRunning = false;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        demo.updateAndRender(); 

        LARGE_INTEGER endPerfCount{};
        QueryPerformanceCounter(&endPerfCount);

        double elapsedTicks = (double)(endPerfCount.QuadPart - beginPerfCount.QuadPart);
        double performanceDeltaSeconds = elapsedTicks /  
                                   ((double)(perfCounterFrequency.QuadPart));
        
        //save frame time in seconds 
        demo.lastFrameTime = (float)(performanceDeltaSeconds);
    }

    demo.shutDown();
    
    return 0;
}