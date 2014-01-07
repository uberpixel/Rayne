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
		
		RNAPI Screen *GetScreen() const { return _screen; }
		RNAPI uint32 GetWidth()  const { return _width; }
		RNAPI uint32 GetHeight() const { return _height; }
		
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
		
		RNAPI ~Screen();
		
		RNAPI uint32 GetWidth() const { return _width; }
		RNAPI uint32 GetHeight() const { return _height; }
		
		RNAPI float GetScaleFactor() const { return _scaleFactor; }
		RNAPI const Rect GetFrame() const { return _frame; }
		
		RNAPI const Array& GetConfigurations() const { return _configurations; }
		
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
	
	class Window : public ISingleton<Window>
	{
	public:
		friend class Kernel;

		enum
		{
			MaskFullscreen = (1 << 0),
			MaskVSync = (1 << 1)
		};
		typedef uint32 Mask;
		
		RNAPI Window();
		RNAPI ~Window() override;

		RNAPI void SetTitle(const std::string& title);
		RNAPI void SetConfiguration(const WindowConfiguration *configuration, Mask mask);
		
		RNAPI void ShowCursor();
		RNAPI void HideCursor();
		
		RNAPI void CaptureMouse();
		RNAPI void ReleaseMouse();

		RNAPI Rect GetFrame() const;
		
		RNAPI Screen *GetActiveScreen() const { return _activeScreen; }
		RNAPI Screen *GetMainScreen() const { return _mainScreen; }
		
#if RN_PLATFORM_MAC_OS
		Screen *GetScreenWithID(CGDirectDisplayID display);
#endif
		
		RNAPI const WindowConfiguration *GetConfiguration() const { return _activeConfiguration; }
		RNAPI const std::vector<Screen *>& GetScreens() const { return _screens; }

	private:
		PIMPL<WindowInternals> _internals;

		Context *_context;
		Kernel  *_kernel;
		
		std::string _title;
		Mask _mask;
		
		bool _cursorVisible;
		bool _mouseCaptured;
		
		std::vector<Screen *> _screens;
		Screen *_mainScreen;
		Screen *_activeScreen;
		
		WindowConfiguration *_activeConfiguration;
		
		RNDefineSingleton(Window)
	};
}

#endif /* __RAYNE_WINDOW_H__ */
