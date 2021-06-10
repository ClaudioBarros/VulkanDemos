#ifndef VULKAN_MANAGER_H
#define VULKAN_MANAGER_H

#include "typedefs_and_macros.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::vector<VkFramebuffer> swapchainFramebuffers;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

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

    uint32 getMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties);

    void createSwapchain();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers(); 
    void createCommandPool();
    void createVertexBuffer();
    void createCommandBuffers();
    void createSyncPrimitives();

    void destroySwapchain();

    void resize();

    void renderFrame();

};

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;
};

#endif