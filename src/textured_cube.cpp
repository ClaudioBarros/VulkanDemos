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

void Demo::startUp()
{
    //geometry data
    vertexData = vertices;
    uvData = texCoords; 

    //load texture files
    textures.resize(1);
    vulkanTextures.resize(1);
    textures[0].load(std::string("path_to_string"));

    //====== Vulkan configuration ===== 
    VulkanConfig vulkanConfig{};
    vulkanConfig.appName = "Textured Cube";

    // --- validation layers ---
    vulkanConfig.enableValidationLayers = true;
    vulkanConfig.validationLayers.push_back("VK_LAYER_KHRONOS_validation");

    //--- device extensions ---
    vulkanConfig.deviceExtensions.push_back("VK_KHR_swapchain");

    //--- formats ---
    vulkanConfig.preferredDepthFormat = VK_FORMAT_D16_UNORM;
    vulkanConfig.preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    vulkanConfig.preferredSurfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
    vulkanConfig.preferredSurfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    vulkanConfig.texFormat = VK_FORMAT_R8G8B8A8_UNORM;

    //===================================
    //vulkanManager.startUp(&window, );
}

void Demo::shutDown()
{

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

    vulkanManager.swapchain.createSwapchainAndImageResources(vulkanManager.surface,
                                                             vulkanManager.logicalDevice.device);
    
    if(vulkanManager.swapchain.imageExtent.width == 0 || 
       vulkanManager.swapchain.imageExtent.height == 0)
    {
        isMinimized = true;
    }
    else
    {
        isMinimized = false;
    }

    if(isMinimized)
    {
        prepared = false;
        return;
    }

    vulkanManager.initDepthImage(); 

    //init textures
    vulkanManager.initVulkanTexture(textures[0].pixels, 
                                    textures[0].width, 
                                    textures[0].height,
                                    stagingTexture.buffer,
                                    stagingTexture.mem,
                                    vulkanTextures[0]);

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
        
        VkCommandBufferAllocateInfo presentCmdAllocInfo;
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
    } 
    
    //flush pipeline commands before beginning the render loop 
    flushInitCmd();
    if(stagingTexture.buffer) vulkanManager.freeVulkanTexture(stagingTexture);
    
    currBufferIndex = 0;
    prepared = true;
}


void Demo::initCubeDataBuffers()
{
    VS_UBO data{};
    
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    data.mvp = camera.projMatrix * camera.viewMatrix * modelMatrix;

    //vulkan expects the y coord to be flipped
    data.mvp[1][1] *= -1;

    for(size_t i = 0; i < vertexData.size(); i++)
    {
        data.pos = glm::vec4(vertexData[i * 3],
                             vertexData[i * 3 + 1],
                             vertexData[i * 3 + 2],
                             1.0f);

        data.uv = glm::vec2(texCoords[i * 2],
                            texCoords[i * 2 + 1]);
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
    depthInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthInfo.depthBoundsTestEnable = VK_FALSE;
    depthInfo.minDepthBounds = 0.0f;
    depthInfo.maxDepthBounds = 1.0f;
    depthInfo.stencilTestEnable = VK_FALSE;
    depthInfo.front = {};
    depthInfo.back = {};

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
    
    std::string vsFilename = "textured_cube_vs.spv";
    std::string fsFilename = "textured_cube_fs.spv";
    
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
    poolSizes[1].descriptorCount = vulkanManager.swapchain.imageCount * textures.size();

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
    VkWriteDescriptorSet writeDescriptorSets[2] = {};

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
    writeDescriptorSets[1].descriptorCount = vulkanTextures.size();
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

    VkFramebufferCreateInfo fbInfo; 
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = vulkanManager.renderPass;
    fbInfo.attachmentCount = 2;
    fbInfo.pAttachments = attachments;
    fbInfo.width = vulkanManager.swapchain.imageExtent.width;
    fbInfo.height = vulkanManager.swapchain.imageExtent.height;
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
    rpBeginInfo.renderArea.extent = vulkanManager.swapchain.imageExtent;
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
    VkViewport vp{};
    vp.width = (float) vulkanManager.swapchain.imageExtent.width;
    vp.height = (float) vulkanManager.swapchain.imageExtent.height;
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    
    VkRect2D scissor{};
    scissor.extent.width = (float) vulkanManager.swapchain.imageExtent.width;
    scissor.extent.height = (float) vulkanManager.swapchain.imageExtent.height;

    vkCmdSetViewport(cmdBuffer, 0, 1, &vp);
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
    
    vkCmdDraw(cmdBuffer, (uint32)(vertexData.size()), 1, 0, 0);

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
    
}

void Demo::updateDataBuffer()
{
    
}

void Demo::draw()
{
    //Make sure that at most MAX_FRAMES rendering are happening at the same time.
    
    vkWaitForFences(vulkanManager.logicalDevice.device, 1, &fences[frameIndex], VK_TRUE, UINT64_MAX);
    vkResetFences(vulkanManager.logicalDevice.device, 1, &fences[frameIndex]);
    
    VkResult err;
    do
    {
        //get the index of the next available swapchain image:
        err = 
        vkAcquireNextImageKHR(vulkanManager.logicalDevice.device, 
                              vulkanManager.swapchain.swapchain,
                              UINT64_MAX,
                              imageAcquiredSemaphores[frameIndex], 
                              VK_NULL_HANDLE, 
                              &currBufferIndex);
        
        if (err == VK_ERROR_OUT_OF_DATE_KHR)
        {
            //swapchain is out of date (window resized, etc) and must be recreated.
            resize();
        }
        
        else if (err == VK_SUBOPTIMAL_KHR)
        {
            //swapchain is not optimal but the image will be correctly presented.
            break;
        }
        else if (err == VK_ERROR_SURFACE_LOST_KHR)
        {
            vkDestroySurfaceKHR(vulkanManager.instance, vulkanManager.surface, nullptr);
            vulkanManager.initSurface(&window);
            resize();
        }
        else
        {
            VK_CHECK(err);
        }
    } while (err != VK_SUCCESS);

    updateDataBuffer();  //TODO

    VkPipelineStageFlags pipelineStageFlags{}; 
    pipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.pWaitDstStageMask = &pipelineStageFlags;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAcquiredSemaphores[frameIndex];
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkanManager.swapchain.imageResources[currBufferIndex].cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &drawCompleteSemaphores[frameIndex];

    VK_CHECK(vkQueueSubmit(vulkanManager.logicalDevice.graphicsQueue, 
                           1, &submitInfo, 
                           fences[frameIndex]));

    if(vulkanManager.physicalDevice.separatePresentQueue)
    {
        //if separate queues are being used, change image ownership to the present queue
        //before presenting, waiting for the draw complete semaphore and signaling the 
        //ownership released semaphore when finished

        VkFence nullFence = VK_NULL_HANDLE; 
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = drawCompleteSemaphores[frameIndex];
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = vulkanManager.swapchain.imageResources[currBufferIndex].graphicsToPresentCmd;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = imageOwnershipSemaphores[frameIndex];
        
        
        VK_CHECK(vkQueueSubmit(vulkanManager.logicalDevice.graphicsQueue, 
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
        presentInfo.pWaitSemaphores = &imageOwnershipSemaphores[frameIndex];
    }
    else
    {
        presentInfo.pWaitSemaphores = &drawCompleteSemaphores[frameIndex];
    }

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vulkanManager.swapchain.swapchain;
    presentInfo.pImageIndices = &currBufferIndex;
    
    err = vkQueuePresentKHR(vulkanManager.logicalDevice.presentQueue, &presentInfo);
    frameIndex += 1;
    frameIndex %= MAX_FRAMES;

    if (err == VK_ERROR_OUT_OF_DATE_KHR)
    {
        //swapchain is out of date (window resized, etc) and must be recreated.
        resize();
    }
    
    else if (err == VK_SUBOPTIMAL_KHR)
    {
        //swapchain is not optimal but the image will be correctly presented.
        break;
    }
    else if (err == VK_ERROR_SURFACE_LOST_KHR)
    {
        vkDestroySurfaceKHR(vulkanManager.instance, vulkanManager.surface, nullptr);
        vulkanManager.initSurface(&window);
        resize();
    }
    else
    {
        VK_CHECK(err);
    }
}

void Demo::run()
{
    if(!prepared) return;
    
    draw();
}

//================== Texture =========================

void Texture::load(std::string filepath)
{
    int texWidth, texHeight, texChannels;
    pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
}

void Texture::free()
{
    width = 0;
    height = 0;
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

