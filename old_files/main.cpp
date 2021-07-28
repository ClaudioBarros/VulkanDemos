#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "textured_cube.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   PWSTR pCmdLine, int nCmdShow)
{
    Demo demo;     
    demo.startUp();
    
    demo.prepare();

    MSG msg{};

    bool isRunning = false;
    while(isRunning)
    {

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
        
        RedrawWindow(demo.window.handle, nullptr, nullptr, RDW_INTERNALPAINT);
    }
    
    demo.shutDown();
    
}