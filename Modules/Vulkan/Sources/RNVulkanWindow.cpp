//
//  RNVulkanWindow.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanWindow.h"
#include "RNVulkanRenderer.h"
#include "RNVulkanSwapChain.h"
#include "RNVulkanFramebuffer.h"

namespace RN
{
#if RN_PLATFORM_WINDOWS
	static std::once_flag __windowClassToken;
	static WNDCLASSEXW __windowClass;
#endif

	RNDefineMeta(VulkanWindow, Window);

	VulkanWindow::VulkanWindow(const Vector2 &size, Screen *screen, VulkanRenderer *renderer, const Window::SwapChainDescriptor &descriptor) :
		Window(screen),
		_renderer(renderer),
		_swapChain(nullptr)
	{
#if RN_PLATFORM_WINDOWS
		HINSTANCE hInstance = ::GetModuleHandle(nullptr);

		std::call_once(__windowClassToken, [&] {

			__windowClass = { 0 };
			__windowClass.cbSize = sizeof(WNDCLASSEX);
			__windowClass.style = CS_HREDRAW | CS_VREDRAW;
			__windowClass.lpfnWndProc = &DefWindowProcW;
			__windowClass.hInstance = hInstance;
			__windowClass.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
			__windowClass.lpszClassName = L"RNVulkanWindowClass";

			::RegisterClassExW(&__windowClass);

		});

		// Create the window
		const DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_OVERLAPPEDWINDOW;

		RECT windowRect = { 0, 0, static_cast<LONG>(size.x), static_cast<LONG>(size.y) };
		::AdjustWindowRect(&windowRect, style, FALSE);

		Rect frame = screen->GetFrame();

		Vector2 offset = Vector2(frame.width - (windowRect.right - windowRect.left), frame.height - (windowRect.bottom - windowRect.top));
		offset *= 0.5;
		offset.x += frame.x;
		offset.y += frame.y;

		_hwnd = ::CreateWindowExW(0, L"RNVulkanWindowClass", L"", style, offset.x, offset.y, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, hInstance, this);

		::SetForegroundWindow(_hwnd);

		// Create the swap chain
        _swapChain = new VulkanSwapChain(size, _hwnd, renderer, descriptor);
#endif
#if RN_PLATFORM_LINUX

		xcb_connection_t *connection = Kernel::GetSharedInstance()->GetXCBConnection();
		const xcb_screen_t *xcbscreen = screen->GetXCBScreen();

		uint32_t value_mask, value_list[32];

		value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
		value_list[0] = xcbscreen->black_pixel;
		value_list[1] = XCB_EVENT_MASK_STRUCTURE_NOTIFY;

		_window = xcb_generate_id(connection);

		xcb_create_window(connection, XCB_COPY_FROM_PARENT, _window, xcbscreen->root, 0, 0, static_cast<uint16_t>(size.x), static_cast<uint16_t>(size.y), 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, xcbscreen->root_visual, value_mask, value_list);


		xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, 1, 12, "WM_PROTOCOLS");
		xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookie, 0);

		cookie = xcb_intern_atom(connection, 0, 16, "WM_DELETE_WINDOW");
		_destroyWindow = xcb_intern_atom_reply(connection, cookie, 0);

		xcb_change_property(connection, XCB_PROP_MODE_REPLACE, _window, (*reply).atom, 4, 32, 1, &(*_destroyWindow).atom);
		free(reply);

		if(descriptor.wantsFullscreen)
        {
		    xcb_intern_atom_cookie_t stateCookie = xcb_intern_atom(connection, false, 13, "_NET_WM_STATE");
		    xcb_intern_atom_reply_t *stateReply = xcb_intern_atom_reply(connection, stateCookie, NULL);

            xcb_intern_atom_cookie_t fullscreenCookie = xcb_intern_atom(connection, false, 24, "_NET_WM_STATE_FULLSCREEN");
            xcb_intern_atom_reply_t *fullscreenReply = xcb_intern_atom_reply(connection, fullscreenCookie, NULL);

            xcb_change_property(connection, XCB_PROP_MODE_REPLACE, _window, stateReply->atom, XCB_ATOM_ATOM, 32, 1, &(fullscreenReply->atom));

            free(stateReply);
            free(fullscreenReply);
        }

		xcb_map_window(connection, _window);


		Rect frame = screen->GetFrame();

		Vector2 offset = Vector2(frame.width - size.x, frame.height - size.y);
		offset *= 0.5;
		offset += frame.GetOrigin();

		const uint32_t coords[] = { static_cast<uint32_t>(offset.x), static_cast<uint32_t>(offset.y) };
		xcb_configure_window(connection, _window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);

		xcb_flush(connection);

		// Create the swap chain
        _swapChain = new VulkanSwapChain(size, _window, renderer, descriptor);
#endif
#if RN_PLATFORM_ANDROID
		_window = Kernel::GetSharedInstance()->GetAndroidApp()->window;

		// Create the swap chain
        _swapChain = new VulkanSwapChain(GetSize(), _window, renderer, descriptor);
#endif
	}

	VulkanWindow::~VulkanWindow()
	{
#if RN_PLATFORM_WINDOWS
		::DestroyWindow(_hwnd);
#endif
		SafeRelease(_swapChain);
	}

	void VulkanWindow::SetTitle(const String *title)
	{
#if RN_PLATFORM_WINDOWS
		char *text = title->GetUTF8String();
		wchar_t *wtext = new wchar_t[strlen(text) + 1];
		mbstowcs(wtext, text, strlen(text) + 1);//Plus null
		LPWSTR ptr = wtext;

		::SetWindowTextW(_hwnd, ptr);

		delete[] wtext;
#endif
#if RN_PLATFORM_LINUX
		xcb_change_property(Kernel::GetSharedInstance()->GetXCBConnection(), XCB_PROP_MODE_REPLACE, _window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, title->GetLength(), title->GetUTF8String());
#endif
	}

	Screen *VulkanWindow::GetScreen()
	{
#if RN_PLATFORM_WINDOWS
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
#endif
#if RN_PLATFORM_LINUX
		xcb_connection_t *connection = Kernel::GetSharedInstance()->GetXCBConnection();
		xcb_get_geometry_reply_t *geometry = xcb_get_geometry_reply(connection, xcb_get_geometry(connection, _window), nullptr);

		Screen *result = nullptr;
		Array *screens = Screen::GetScreens();

		screens->Enumerate<Screen>([&](Screen *screen, size_t index, bool &stop) {

			if(screen->GetXCBScreen()->root == geometry->root)
			{
				result = screen;
				stop = true;
			}

		});

		free(geometry);

		return result;
#endif

		return nullptr;
	}

	void VulkanWindow::Show()
	{
#if RN_PLATFORM_WINDOWS
		::ShowWindow(_hwnd, SW_SHOW);
#endif
#if RN_PLATFORM_LINUX
		xcb_map_window(Kernel::GetSharedInstance()->GetXCBConnection(), _window);
#endif
	}

	void VulkanWindow::Hide()
	{
#if RN_PLATFORM_WINDOWS
		::ShowWindow(_hwnd, SW_HIDE);
#endif
#if RN_PLATFORM_LINUX
		xcb_unmap_window(Kernel::GetSharedInstance()->GetXCBConnection(), _window);
#endif
	}

	void VulkanWindow::SetFullscreen(bool fullscreen)
	{
		_swapChain->SetFullscreen(fullscreen);

	}

	Vector2 VulkanWindow::GetSize() const
	{
#if RN_PLATFORM_WINDOWS
		RECT windowRect;
		::GetClientRect(_hwnd, &windowRect);
		return Vector2(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
#endif
#if RN_PLATFORM_LINUX
		xcb_connection_t *connection = Kernel::GetSharedInstance()->GetXCBConnection();
		xcb_get_geometry_reply_t *geometry = xcb_get_geometry_reply(connection, xcb_get_geometry(connection, _window), nullptr);

		Vector2 size(geometry->width, geometry->height);

		free(geometry);

		return size;
#endif
#if RN_PLATFORM_ANDROID
		int width = ANativeWindow_getWidth(_window);
		int height = ANativeWindow_getHeight(_window);
		return Vector2(width, height);
#endif
	}

	Framebuffer *VulkanWindow::GetFramebuffer() const
	{
		return _swapChain->GetFramebuffer();
	}

	void VulkanWindow::UpdateSize()
	{
		_swapChain->ResizeSwapchain(GetSize());
		NotificationManager::GetSharedInstance()->PostNotification(kRNWindowDidChangeSize, this);
	}

	const Window::SwapChainDescriptor &VulkanWindow::GetSwapChainDescriptor() const
	{
		return _swapChain->GetSwapChainDescriptor();
	}

	uint64 VulkanWindow::GetWindowHandle() const
	{
#if RN_PLATFORM_WINDOWS
		return reinterpret_cast<uint64>(_hwnd);
#elif RN_PLATFORM_LINUX
		return _window;
#elif RN_PLATFORM_ANDROID
		return reinterpret_cast<uint64>(_window);
#endif
	}
}
