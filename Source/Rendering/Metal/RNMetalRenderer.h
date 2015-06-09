//
//  RNMetalRenderer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALRENDERER_H__
#define __RAYNE_METALRENDERER_H__

#include "../../Base/RNBase.h"
#include "../RNRenderer.h"
#include "RNMetalWindow.h"

namespace RN
{
	class MetalRenderer : public Renderer
	{
	public:
		MetalRenderer();
		Window *CreateWindow(const Rect &frame, Screen *screen) final;

	protected:
		MetalWindow *_mainWindow;
	};
}


#endif /* __RAYNE_METALRENDERER_H__ */
