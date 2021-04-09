
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "game.h"

void Game::startUp()
{
    renderManager.startUp();
}

void Game::shutDown()
{
    renderManager.shutDown();
}

int main()
{
    Game game;
    game.startUp();

    while(!glfwWindowShouldClose(game.renderManager.window)) {
        glfwPollEvents();
    }

    return 0;
}