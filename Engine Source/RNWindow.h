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
		
		RNAPI WindowConfiguration(uint32 width, uint32 height);
		RNAPI WindowConfiguration(uint32 width, uint32 height, Screen *screen);
		
		Screen *GetScreen() const { return _screen; }
		uint32 GetWidth()  const { return _width; }
		uint32 GetHeight() const { return _height; }
		
	private:
		Screen *_screen;
		
		uint32 _width;
		uint32 _height;
		
		RNDefineMeta(WindowConfiguration, Object)
	};
	
	class Screen
	{
	public:
		friend class Window;
		
		~Screen();
		
		uint32 GetWidth() const { return _width; }
		uint32 GetHeight() const { return _height; }
		
		float GetScaleFactor() const { return _scaleFactor; }
		const Rect GetFrame() const { return _frame; }
		
		const Array& GetConfigurations() const { return _configurations; }
		
	private:
#if RN_PLATFORM_MAC_OS
		Screen(CGDirectDisplayID display);
#endif
		
#if RN_PLATFORM_WINDOWS
		Screen(const char *name);
#endif
		
		uint32 _width;
		uint32 _height;
		
		Rect _frame;
		float _scaleFactor;
		
#if RN_PLATFORM_MAC_OS
		CGDirectDisplayID _display;
#endif
		
#if RN_PLATFORM_WINDOWS
		std::string _display;
#endif
		
		Array _configurations;
	};
	
	class Kernel;
	struct WindowInternals;
	
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
		RNAPI ~Window() override;

		RNAPI void SetTitle(const std::string& title);
		RNAPI void SetConfiguration(const WindowConfiguration *configuration, WindowMask mask);
		
		RNAPI void ShowCursor();
		RNAPI void HideCursor();
		
		RNAPI void CaptureMouse();
		RNAPI void ReleaseMouse();

		RNAPI Rect GetFrame() const;
		
		Screen *GetActiveScreen() const { return _activeScreen; }
		Screen *GetMainScreen() const { return _mainScreen; }
		
#if RN_PLATFORM_MAC_OS
		Screen *GetScreenWithID(CGDirectDisplayID display);
#endif
		
		const WindowConfiguration *GetConfiguration() const { return _activeConfiguration; }
		const std::vector<Screen *>& GetScreens() const { return _screens; }

	private:
		PIMPL<WindowInternals> _internals;

		Context *_context;
		Kernel  *_kernel;
		
		std::string _title;
		WindowMask _mask;
		
		bool _cursorVisible;
		bool _mouseCaptured;
		
		std::vector<Screen *> _screens;
		Screen *_mainScreen;
		Screen *_activeScreen;
		
		WindowConfiguration *_activeConfiguration;
	};
}

#endif /* __RAYNE_WINDOW_H__ */
