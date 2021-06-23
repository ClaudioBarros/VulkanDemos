#ifndef PLATFORM_H
#define PLATFORM 

#include <windows.h>
#include <string>

struct Win32Window
{
	HWND window;
	HINSTANCE hInstance;
	std::wstring name;

	uint32 width;
	uint32 height;
	
	uint32 minWidth;
	uint32 minHeight;

	static 	LRESULT CALLBACK windowProc(HWND   hwnd,
										UINT   uMsg,
										WPARAM wParam,
										LPARAM lParam);

	Window(){};	
	~Window(){};

	void init(HINSTANCE hInstance, const std::wstring windowClassName);
	void destroy();
};

#endif