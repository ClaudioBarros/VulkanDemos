#ifndef RENDER_MANAGER_H
#define RENDER_MANAGER_H

#include "typedefs_and_macros.h"
#include "vulkan_manager.h"

#include <vector>

struct RenderManager
{
    GLFWwindow *window;
    VulkanManager vulkanManager;

    RenderManager(){}  
    ~RenderManager(){}
    
    void startUp();
    void shutDown();

    void render();
};

#endif