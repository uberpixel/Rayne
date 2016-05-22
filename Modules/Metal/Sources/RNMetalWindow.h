//
//  RNMetalWindow.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALWINDOW_H__
#define __RAYNE_METALWINDOW_H__

#include "RNMetal.h"

namespace RN
{
	class MetalRenderer;
	struct MetalWindowInternals;

	class MetalWindow : public Window
	{
	public:
		friend class MetalRenderer;

		MTLAPI void SetTitle(const String *title) final;
		MTLAPI Screen *GetScreen() final;

		MTLAPI void Show() final;
		MTLAPI void Hide() final;

		MTLAPI Vector2 GetSize() const final;

	private:
		MetalWindow(const Vector2 &size, Screen *screen, MetalRenderer *renderer);

		PIMPL<MetalWindowInternals> _internals;
		MetalRenderer *_renderer;

		RNDeclareMetaAPI(MetalWindow, MTLAPI)
	};
}

#endif /* __RAYNE_METALWINDOW_H__ */
