#ifndef PLATFORM_H
#define PLATFORM_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include "typedefs_and_macros.h"
 
struct Demo;

struct Win32Window
{
	HWND handle = nullptr;
	HINSTANCE hInstance;
	std::wstring name;

	Demo *demo;	

	uint16 width;
	uint16 height;
		
	uint16 minX;
	uint16 minY;

	static LRESULT CALLBACK StaticWndProc(HWND   hwnd,
										  UINT   uMsg,
										  WPARAM wParam,
										  LPARAM lParam)
	{
		Win32Window *self;
		if(uMsg == WM_NCCREATE)
		{
			LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			self = static_cast<Win32Window *>(lpcs->lpCreateParams);
			self->handle = hwnd;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
		}
		else
		{
			self = reinterpret_cast<Win32Window *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		if(self)
		{
			return self->WndProc(uMsg, wParam, lParam);
		}

		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	//User defined callback function
	LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void init(Demo *pDemo, 
	          const std::wstring windowClassName, 
			  uint16 _width, 
			  uint16 _height)
	{
		this->demo = pDemo; 
		this->name = windowClassName;
		this->width = _width;
		this->height = _height;

		WNDCLASSEX windowClass{};

		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = this->StaticWndProc;
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
									  this);               				// ptr to the class	
		if (!this->handle)
		{
			LOGE_EXIT("Unable to create a Win32 Window.");
		}

		// Window client area size must be at least 1 pixel high to prevent crashes.
		this->minX = GetSystemMetrics(SM_CXMINTRACK);
		this->minY = GetSystemMetrics(SM_CYMINTRACK) + 1;
	}

	void destroy()
	{
		
	}
};

#endif