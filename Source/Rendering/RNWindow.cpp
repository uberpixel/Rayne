//
//  RNWindow.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWindow.h"

namespace RN
{
	RNDefineMeta(Window, Object)

	Window::Window(Screen *screen) :
		_screen(screen)
	{
		SafeRetain(_screen);
	}

	Window::Window() :
		_screen(nullptr)
	{}

	Window::~Window()
	{
		SafeRelease(_screen);
	}
}
