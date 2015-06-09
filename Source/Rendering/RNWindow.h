//
//  RNWindow.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_WINDOW_H_
#define __RAYNE_WINDOW_H_

#include "../Base/RNBase.h"
#include "../System/RNScreen.h"

namespace RN
{
	class Window
	{
	public:
		virtual void SetTitle(const String *title) = 0;

	protected:
		RNAPI Window(Screen *screen);

	private:
		Screen *_screen;
	};
}


#endif /* __RAYNE_WINDOW_H_ */
