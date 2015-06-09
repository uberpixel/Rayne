//
//  RNRenderer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_RENDERER_H_
#define __RAYNE_RENDERER_H_

#include "../Base/RNBase.h"
#include "../System/RNScreen.h"
#include "RNWindow.h"

namespace RN
{
	class Renderer
	{
	public:
		RNAPI static Renderer *GetActiveRenderer();

		RNAPI virtual Window *CreateWindow(const Rect &frame, Screen *screen) = 0;
		RNAPI void Activate();

	protected:
		RNAPI Renderer();
	};
}


#endif /* __RAYNE_RENDERER_H_ */
