#ifndef GAME_H
#define GAME_H

#include "typedefs_and_macros.h"
#include "render_manager.h"

struct Game 
{
    RenderManager renderManager;

    Game(){}
    ~Game(){}

    void startUp();
    void shutDown();
};

#endif