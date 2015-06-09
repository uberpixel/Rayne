//
//  RNMetalWindow.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALWINDOW_H__
#define __RAYNE_METALWINDOW_H__

#include "../../Base/RNBase.h"
#include "../RNWindow.h"

namespace RN
{
	class MetalWindow : public Window
	{
	public:
		MetalWindow(const Rect &frame, Screen *screen);

		void SetTitle(const String *title) final;

	private:
		struct Internals;
		PIMPL<Internals> _internals;
	};
}

#endif /* __RAYNE_METALWINDOW_H__ */
