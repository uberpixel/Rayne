//
//  RND3D12Window.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12Internals.h"
#include "RND3D12Window.h"
#include "RND3D12Renderer.h"

namespace RN
{
	RNDefineMeta(D3D12Window, Window)

	D3D12Window::D3D12Window(const Vector2 &size, Screen *screen, D3D12Renderer *renderer) : Window(screen), _renderer(nullptr)
	{
		HINSTANCE hInstance = GetModuleHandle(NULL);;

		// Initialize the window class.
		WNDCLASSEXW windowClass = { 0 };
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = &WindowProc;
		windowClass.hInstance = hInstance;
		windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		windowClass.lpszClassName = L"RND3D12WindowClass";
		RegisterClassExW(&windowClass);

		RECT windowRect = { 0, 0, static_cast<LONG>(size.x), static_cast<LONG>(size.y) };
		AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

		// Create the window and store a handle to it.
		_hwnd = CreateWindowExW(NULL,
			L"RND3D12WindowClass",
			L"",
			WS_OVERLAPPEDWINDOW,
			300,
			300,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			NULL,		// We have no parent window, NULL.
			NULL,		// We aren't using menus, NULL.
			hInstance,
			this);		// We aren't using multiple windows, NULL.

		ShowWindow(_hwnd, SW_SHOWDEFAULT);
	}

	void D3D12Window::SetTitle(const String *title)
	{
		char *text = title->GetUTF8String();
		wchar_t *wtext = new wchar_t[strlen(text) + 1];
		mbstowcs(wtext, text, strlen(text) + 1);//Plus null
		LPWSTR ptr = wtext;

		SetWindowTextW(_hwnd, ptr);

		delete[] wtext;
	}

	Screen *D3D12Window::GetScreen()
	{
		//TODO: Needs implementing!

//		Array *screens = Screen::GetScreens();
//		NSScreen *nsscreen = [_internals->window screen];

		Screen *result = nullptr;

/*		screens->Enumerate<Screen>([&](Screen *screen, size_t index, bool &stop) {

			if(screen->GetNSScreen() == nsscreen)
			{
				result = screen;
				stop = true;
			}

		});*/

		RN_ASSERT(result, "Result must not be NULL, something broke internally");
		return result;
	}

	void D3D12Window::Show()
	{
		ShowWindow(_hwnd, SW_SHOW);
	}

	void D3D12Window::Hide()
	{
		ShowWindow(_hwnd, SW_HIDE);
	}

	Vector2 D3D12Window::GetSize() const
	{
		RECT windowRect;
		GetClientRect(_hwnd, &windowRect);
		return Vector2(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);

		return Vector2();
	}
}
