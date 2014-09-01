//
//  RNWindow.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WINDOW_H__
#define __RAYNE_WINDOW_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNArray.h"
#include "RNRect.h"
#include "RNMessage.h"
#include "RNEnum.h"

#define kRNWindowConfigurationChanged   RNCSTR("kRNWindowConfigurationChanged")
#define kRNWindowScaleFactorChanged     RNCSTR("kRNWindowScaleFactorChanged")
#define kRNWindowScreenChanged          RNCSTR("kRNWindowScreenChanged")


namespace RN
{
	class Window;
	class Screen;
	
	class WindowConfiguration : public Object
	{
	public:
		friend class Window;
		friend class Screen;
		
		RNAPI WindowConfiguration(const WindowConfiguration *other);
		RNAPI WindowConfiguration(uint32 width, uint32 height);
		RNAPI WindowConfiguration(uint32 width, uint32 height, Screen *screen);
		
		RNAPI Screen *GetScreen() const { return _screen; }
		RNAPI uint32 GetWidth()  const { return _width; }
		RNAPI uint32 GetHeight() const { return _height; }
		
	private:
		Screen *_screen;
		
		uint32 _width;
		uint32 _height;
		
		RNDeclareMeta(WindowConfiguration)
	};
	
	class Screen
	{
	public:
		friend class Window;
		
		RNAPI ~Screen();
		
		RNAPI uint32 GetWidth() const { return _frame.width; }
		RNAPI uint32 GetHeight() const { return _frame.height; }
		
		RNAPI float GetScaleFactor() const { return _scaleFactor; }
		RNAPI const Rect &GetFrame() const { return _frame; }
		
		RNAPI const Array *GetConfigurations() const { return &_configurations; }
		
	private:
#if RN_PLATFORM_MAC_OS
		Screen(CGDirectDisplayID display);
#endif
#if RN_PLATFORM_WINDOWS
		Screen(HMONITOR monitor);
		bool IsMainScreen() const { return _isMain; }
#endif
		Rect  _frame;
		float _scaleFactor;
		
#if RN_PLATFORM_MAC_OS
		CGDirectDisplayID _display;
#endif
#if RN_PLATFORM_WINDOWS
		HMONITOR _monitor;
		bool _isMain;
#endif
		
		Array _configurations;
	};
	
	class Kernel;
	struct WindowInternals;
	
	class Window : public ISingleton<Window>
	{
	public:
		friend class Kernel;

		struct Mask : public Enum<uint32>
		{
			Mask()
			{}
			
			Mask(int val) :
				Enum(val)
			{}
			
			enum
			{
				Fullscreen = (1 << 0),
				VSync      = (1 << 1),
				Borderless = (1 << 2)
			};
		};
		
		RNAPI Window();
		RNAPI ~Window() override;

		RNAPI void SetTitle(const std::string &title);
		RNAPI void SetPosition(const Vector2 &position);
		RNAPI void ActivateConfiguration(WindowConfiguration *configuration, Mask mask);
		
		RNAPI void ShowCursor();
		RNAPI void HideCursor();
		
		RNAPI void CaptureMouse();
		RNAPI void ReleaseMouse();

		RNAPI Vector2 GetSize() const;
		
		RNAPI Screen *GetActiveScreen() const { return _activeScreen; }
		RNAPI Screen *GetMainScreen() const { return _mainScreen; }
		
#if RN_PLATFORM_MAC_OS
		Screen *GetScreenWithID(CGDirectDisplayID display);
#elif RN_PLATFORM_WINDOWS
		RNAPI HWND GetCurrentWindow() const;
		RNAPI HDC GetCurrentDC() const;
#endif
		
		RNAPI WindowConfiguration *GetActiveConfiguration() const { return _activeConfiguration; }
		RNAPI const std::vector<Screen *> &GetScreens() const { return _screens; }
		
		RNAPI void SetFlushProc(std::function<void()> flush);

	private:
		void Flush();
		
		std::function<void()> _flushProc;
		
		PIMPL<WindowInternals> _internals;
		Kernel *_kernel;
		
		std::string _title;
		Mask _mask;
		
		bool _cursorVisible;
		bool _mouseCaptured;
		
		std::vector<Screen *> _screens;
		Screen *_mainScreen;
		Screen *_activeScreen;
		
		WindowConfiguration *_activeConfiguration;
		
		RNDeclareSingleton(Window)
	};
}

#endif /* __RAYNE_WINDOW_H__ */
