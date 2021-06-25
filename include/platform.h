#ifndef PLATFORM_H
#define PLATFORM 

#include <windows.h>
#include <string>
#include "typedefs_and_macros.h"
 
struct Win32Window
{
	HWND handle;
	HINSTANCE hInstance;
	std::wstring name;

	uint32 width;
	uint32 height;
	
	uint32 minX;
	uint32 minY;

	static 	LRESULT CALLBACK windowProc(HWND   hwnd,
										UINT   uMsg,
										WPARAM wParam,
										LPARAM lParam);

	void init(HINSTANCE hInstance, const std::wstring windowClassName);
	void destroy();
};

#endif