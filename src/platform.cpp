#include "platform.h"
#include <cstdlib>
#include "typedefs_and_macros.h"

void Win32Window::init(HINSTANCE hInstance, const std::wstring windowClassName)
{
	this->hInstance = hInstance;
	this->name = windowClassName;

	WNDCLASSEX windowClass{};

    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = this->windowProc;
    windowClass.hInstance = hInstance;  
    windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    windowClass.lpszClassName = windowClassName.data();

    // Register window class:
    if (!RegisterClassEx(&windowClass)) 
	{
		LOGE_EXIT("Unable to register Win32 Window Class.");
    }

    RECT wr = {0, 0, this->width, this->height};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    this->handle = CreateWindowEx(0,
                                  windowClassName.data(),           // window class name 
                                  windowClassName.data(), 			// window text 
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE, // window style
                                  CW_USEDEFAULT, CW_USEDEFAULT,     // x/y coords
                                  (wr.right - wr.left),  		    // width
                                  (wr.bottom - wr.top),  			// height
                                  0,               			        // handle to parent
                                  0,               			        // handle to menu
                                  this->hInstance,    			    // hInstance
                                  0);               				// no extra parameters
    if (!this->handle)
	{
        LOGE_EXIT("Unable to create a Win32 Window.");
    }

    // Window client area size must be at least 1 pixel high to prevent crashes.
    this->minX = GetSystemMetrics(SM_CXMINTRACK);
    this->minY = GetSystemMetrics(SM_CYMINTRACK) + 1;
}

void Win32Window::destroy()
{
    //Windows are destroyed automatically by the OS once the program exits.
}
