//
//  RNWindow.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WINDOW_H__
#define __RAYNE_WINDOW_H__

#include "RNBase.h"

#include "RNObject.h"
#include "RNContext.h"

namespace RN
{
	class Window;
	class WindowConfiguration
	{
	friend class Window;
	public:
#if RN_PLATFORM_MAC_OS
		WindowConfiguration(CGDisplayModeRef mode);
		~WindowConfiguration();
#endif
#if RN_PLATFORM_LINUX
		WindowConfiguration(int32 index, uint32 width, uint32 height);
#endif
		
		WindowConfiguration(uint32 width, uint32 height);
		
		uint32 Width() const { return _width; }
		uint32 Height() const { return _height; }
		
	private:
		uint32 _width;
		uint32 _height;
		
#if RN_PLATFORM_MAC_OS
		CGDisplayModeRef _mode;
#endif
		
#if RN_PLATFORM_LINUX
		int32 _modeIndex;
#endif
	};
	
	class Kernel;
	class Window : public Singleton<Window>
	{
	public:
		enum
		{
			WindowMaskFullscreen = (1 << 0),
			WindowMaskVSync = (1 << 1)
		};
		typedef uint32 WindowMask;
		
		Window();
		virtual ~Window();

		void SetTitle(const std::string& title);
		void SetConfiguration(WindowConfiguration *configuration, WindowMask mask);
		
		void ShowCursor();
		void HideCursor();

		WindowConfiguration *ActiveConfiguration() const { return _activeConfiguration; }
		const Array<WindowConfiguration *>& Configurations() const { return _configurations; }
		
		Rect Frame() const;

	private:
#if RN_PLATFORM_MAC_OS
		void *_nativeWindow;
#endif

#if RN_PLATFORM_IOS
		void *_nativeWindow;
		void *_rootViewController;
		void *_renderingView;
#endif
		
#if RN_PLATFORM_LINUX
		Display *_dpy;
		XID _win;
		XRRScreenConfiguration *_screenConfig;
		Cursor _emtpyCursor;
#endif
		
#if RN_PLATFORM_WINDOWS
		HWND _hWnd;
		HDC _hDC;
#endif

		Context *_context;
		Kernel *_kernel;
		
		std::string _title;
		bool _cursorVisible;
		
		WindowMask _mask;
		WindowConfiguration *_activeConfiguration;
		Array<WindowConfiguration *> _configurations;
	};
}

#endif /* __RAYNE_WINDOW_H__ */
