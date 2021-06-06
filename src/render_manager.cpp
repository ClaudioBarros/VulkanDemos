#define GLFW_INCLUDE_VULKAN 
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <iostream>
#include <vector>

#include "render_manager.h"
#include "vulkan_manager.h"

void RenderManager::startUp()
{
    //GLFW
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(1280, 720, "Vulkan window", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, handleResize);

    //Vulkan
    vulkanManager.startUp(window);
}

void RenderManager::shutDown()
{
    //GLFW
    vulkanManager.shutDown();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void RenderManager::handleResize(GLFWwindow *window, int width, int height)
{
    glfwGetWindowUserPointer(window);
    RenderManager* ptrRenderManager = reinterpret_cast<RenderManager*>(glfwGetWindowUserPointer(window));
    ptrRenderManager->vulkanManager.windowWasResized = true;
}

void RenderManager::render()
{
   vulkanManager.renderFrame(); 
}