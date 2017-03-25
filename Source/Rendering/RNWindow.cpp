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
		_screen(screen->Retain())
	{}

	Window::Window() :
		_screen(nullptr)
	{}

	Window::~Window()
	{
		if(_screen)
			_screen->Release();
	}
}
