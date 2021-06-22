#ifndef VULKAN_MANAGER_H
#define VULKAN_MANAGER_H

#include "typedefs_and_macros.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <vector>

struct VulkanManager
{
    GLFWwindow *window;

    VkInstance instance;

    VkSurfaceKHR surface;
     
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    VkPhysicalDeviceFeatures enabledFeatures;
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;

    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    uint32 graphicsQueueFamilyIndex;
    uint32 presentQueueFamilyIndex;

    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainImageExtent;

    std::vector<VkImageView> swapchainImageViews;

    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::vector<VkFramebuffer> swapchainFramebuffers;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;    

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    VkImage textureImg;    
    VkDeviceMemory textureImgMemory;
    VkImageView textureImgView;
    VkSampler textureSampler;

    VkCommandPool cmdPool;
    std::vector<VkCommandBuffer> cmdBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    std::vector<VkFence> queueSubmitFences;
    std::vector<VkFence> imageInUseFences;

    size_t currFrame = 0;           

    bool windowWasResized = false;
    
    void startUp(GLFWwindow *window);
    void shutDown();

    void createImage(uint32 width, uint32 height,
                     VkFormat format, VkImageTiling tiling, 
                     VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
                     VkImage& image, VkDeviceMemory& imageMemory);

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    void transitionImageLayout(VkImage image, VkFormat format, 
                               VkImageLayout oldLayout, VkImageLayout newLayout);

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32 width, uint32 height);

    uint32 getMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties);

    void createBuffer(VkDeviceSize size, 
                      VkBufferUsageFlags usageFlags, 
                      VkMemoryPropertyFlags propertyFlags,
                      VkBuffer &buffer, VkDeviceMemory &bufferMemory);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void updateUniformBuffer(uint32 currImage);


    void createSwapchain();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createFramebuffers(); 
    void createCommandPool();
    void createDepthResources();
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    void createSyncPrimitives();

    void destroySwapchain();

    void resize();

    void renderFrame();

};

struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;
};

#endif