//
//  RNVulkanWindow.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanWindow.h"
#include "RNVulkanRenderer.h"

namespace RN
{
	static std::once_flag __windowClassToken;
	static WNDCLASSEXW __windowClass;

	RNDefineMeta(VulkanWindow, Window);

	VulkanWindow::VulkanWindow(const Vector2 &size, Screen *screen, VulkanRenderer *renderer) :
		Window(screen),
		_renderer(renderer)
	{
		std::call_once(__windowClassToken, [] {

			HINSTANCE hInstance = GetModuleHandle(NULL);

			__windowClass = { 0 };
			__windowClass.cbSize = sizeof(WNDCLASSEX);
			__windowClass.style = CS_HREDRAW | CS_VREDRAW;
			__windowClass.lpfnWndProc = &DefWindowProcW;
			__windowClass.hInstance = hInstance;
			__windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
			__windowClass.lpszClassName = L"RNVulkanWindowClass";

			::RegisterClassExW(&__windowClass);

		});

		HINSTANCE hInstance = GetModuleHandle(NULL);

		RECT windowRect = { 0, 0, static_cast<LONG>(size.x), static_cast<LONG>(size.y) };
		::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);


		_hwnd = ::CreateWindowExW(0, L"RNVulkanWindowClass", L"", WS_OVERLAPPEDWINDOW, 300, 300, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, hInstance, this);

		::ShowWindow(_hwnd, SW_SHOWDEFAULT);
	}

	VulkanWindow::~VulkanWindow()
	{
		::DestroyWindow(_hwnd);
	}

	void VulkanWindow::SetTitle(const String *title)
	{
		char *text = title->GetUTF8String();
		wchar_t *wtext = new wchar_t[strlen(text) + 1];
		mbstowcs(wtext, text, strlen(text) + 1);//Plus null
		LPWSTR ptr = wtext;

		::SetWindowTextW(_hwnd, ptr);

		delete[] wtext;
	}

	Screen *VulkanWindow::GetScreen()
	{
		HMONITOR monitor = MonitorFromWindow(_hwnd, MONITOR_DEFAULTTOPRIMARY);

		Screen *result = nullptr;
		Array *screens = Screen::GetScreens();

		screens->Enumerate<Screen>([&](Screen *screen, size_t index, bool &stop) {

			if(screen->GetHMONITOR() == monitor)
			{
				result = screen;
				stop = true;
			}

		});

		return result;
	}

	void VulkanWindow::Show()
	{
		::ShowWindow(_hwnd, SW_SHOW);
	}

	void VulkanWindow::Hide()
	{
		::ShowWindow(_hwnd, SW_HIDE);
	}

	Vector2 VulkanWindow::GetSize() const
	{
		RECT windowRect;
		::GetClientRect(_hwnd, &windowRect);
		return Vector2(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
	}
}
