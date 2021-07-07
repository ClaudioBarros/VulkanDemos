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

    VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBeginInfo));

    vulkanManager.swapchain.createSwapchainAndImageResources(vulkanManager.surface,
                                                             vulkanManager.logicalDevice.device);

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
    
    
    
}

void Demo::initCubeDataBuffers()
{
    VS_UBO data{};
    data.mvp = camera.projection * camera.view * modelMatrix;

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
                                         &vulkanManager.descriptorSetLayout));
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
    pipelineLayoutInfo.pSetLayouts = &vulkanManager.descriptorSetLayout;
    
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