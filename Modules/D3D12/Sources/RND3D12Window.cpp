//
//  RND3D12Window.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12Window.h"
#include "RND3D12Renderer.h"
#include "RND3D12Framebuffer.h"
#include "RND3D12SwapChain.h"

namespace RN
{
	RNDefineMeta(D3D12Window, Window)

	//TODO: Related to Kernel HandleSystemEvents
	LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_NCCREATE:
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams));
			return DefWindowProcW(hwnd, uMsg, wParam, lParam);

		case WM_SIZE:
		{
			D3D12Window *window = reinterpret_cast<D3D12Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
			if(window)
				window->UpdateSize();
		}

		default:
			return DefWindowProcW(hwnd, uMsg, wParam, lParam);
		}
		return 0;
	}

	D3D12Window::D3D12Window(const Vector2 &size, Screen *screen, D3D12Renderer *renderer, const Window::SwapChainDescriptor &descriptor) :
		Window(screen)
	{
		HINSTANCE hInstance = ::GetModuleHandle(nullptr);

		static std::once_flag flag;
		std::call_once(flag, [&] {

			WNDCLASSEXW windowClass = { 0 };
			windowClass.cbSize = sizeof(WNDCLASSEX);
			windowClass.style = CS_HREDRAW | CS_VREDRAW;
			windowClass.lpfnWndProc = &MainWndProc;
			windowClass.hInstance = hInstance;
			windowClass.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
			windowClass.lpszClassName = L"RND3D12WindowClass";
			RegisterClassExW(&windowClass); 

		});

		const DWORD style =  WS_OVERLAPPEDWINDOW;

		RECT windowRect = { 0, 0, static_cast<LONG>(size.x), static_cast<LONG>(size.y) };
		AdjustWindowRect(&windowRect, style, false);

		Rect frame = screen->GetFrame();

		Vector2 offset = Vector2(frame.width - (windowRect.right - windowRect.left), frame.height - (windowRect.bottom - windowRect.top));
		offset *= 0.5;
		offset.x += frame.x;
		offset.y += frame.y;

		_hwnd = CreateWindowExW(0, L"RND3D12WindowClass", L"", style, offset.x, offset.y, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, hInstance, this);
		SetForegroundWindow(_hwnd);

		_swapChain = new D3D12SwapChain(size, _hwnd, renderer, descriptor);
	}

	D3D12Window::~D3D12Window()
	{
		DestroyWindow(_hwnd);
	}

	void D3D12Window::SetTitle(const String *title)
	{
		char *text = title->GetUTF8String();
		wchar_t *wtext = new wchar_t[title->GetLength() + 1];
		mbstowcs(wtext, text, title->GetLength() + 1);//Plus null
		LPWSTR ptr = wtext;

		SetWindowTextW(_hwnd, ptr);

		delete[] wtext;
	}

	Screen *D3D12Window::GetScreen()
	{
		HMONITOR monitor = ::MonitorFromWindow(_hwnd, MONITOR_DEFAULTTOPRIMARY);

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

	void D3D12Window::Show()
	{
		ShowWindow(_hwnd, SW_SHOW);
	}

	void D3D12Window::Hide()
	{
		ShowWindow(_hwnd, SW_HIDE);
	}

	void D3D12Window::SetFullscreen(bool fullscreen)
	{
		_swapChain->SetFullscreen(fullscreen);
	}

	Vector2 D3D12Window::GetSize() const
	{
		RECT windowRect;
		GetClientRect(_hwnd, &windowRect);
		return Vector2(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
	}

	Framebuffer *D3D12Window::GetFramebuffer() const
	{
		return _swapChain->GetFramebuffer();
	}

	void D3D12Window::UpdateSize()
	{
		_swapChain->ResizeSwapchain(GetSize());
		NotificationManager::GetSharedInstance()->PostNotification(kRNWindowDidChangeSize, this);
	}

	const Window::SwapChainDescriptor &D3D12Window::GetSwapChainDescriptor() const
	{
		return _swapChain->GetSwapChainDescriptor();
	}

	uint64 D3D12Window::GetWindowHandle() const
	{
		return reinterpret_cast<uint64>(_hwnd);
	}
}
