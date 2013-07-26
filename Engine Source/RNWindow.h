//
//  RNWindow.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WINDOW_H__
#define __RAYNE_WINDOW_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNContext.h"
#include "RNArray.h"
#include "RNRect.h"
#include "RNMessage.h"

#define kRNWindowConfigurationChanged RNCSTR("kRNWindowConfigurationChanged")

namespace RN
{
	class Window;
	class WindowConfiguration
	{
	friend class Window;
	public:
#if RN_PLATFORM_MAC_OS
		RNAPI WindowConfiguration(CGDisplayModeRef mode);
		RNAPI WindowConfiguration(const WindowConfiguration& other);
		RNAPI ~WindowConfiguration();
#endif
#if RN_PLATFORM_LINUX
		RNAPI WindowConfiguration(int32 index, uint32 width, uint32 height);
#endif
		
		RNAPI WindowConfiguration(uint32 width, uint32 height);
		
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
		
		RNAPI Window();
		RNAPI virtual ~Window();

		RNAPI void SetTitle(const std::string& title);
		RNAPI void SetConfiguration(const WindowConfiguration& configuration, WindowMask mask);
		
		RNAPI void ShowCursor();
		RNAPI void HideCursor();

		const WindowConfiguration& ActiveConfiguration() const { return _activeConfiguration; }
		const std::vector<WindowConfiguration>& Configurations() const { return _configurations; }
		
		RNAPI Rect Frame() const;

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
		SizeID _originalSize;
		Rotation _originalRotation;
		Cursor _emptyCursor;
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
		WindowConfiguration _activeConfiguration;
		std::vector<WindowConfiguration> _configurations;
	};
}

#endif /* __RAYNE_WINDOW_H__ */
