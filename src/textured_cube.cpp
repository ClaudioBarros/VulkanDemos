#include "textured_cube.h"
     
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

    allocPrimaryCmdBuffer(vulkanManager.logicalDevice.device, vulkanManager.cmdPool, cmdBuffer);

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