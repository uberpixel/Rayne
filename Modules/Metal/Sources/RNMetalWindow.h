//
//  RNMetalWindow.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALWINDOW_H__
#define __RAYNE_METALWINDOW_H__

#include <Rayne.h>

namespace RN
{
	class MetalRenderer;
	struct MetalWindowInternals;

	class MetalWindow : public Window
	{
	public:
		friend class MetalRenderer;

		void SetTitle(const String *title) final;
		Screen *GetScreen() final;

		void Show() final;
		void Hide() final;

		Vector2 GetSize() const final;

	private:
		MetalWindow(const Vector2 &size, Screen *screen, MetalRenderer *renderer);

		PIMPL<MetalWindowInternals> _internals;
		MetalRenderer *_renderer;

		RNDeclareMetaAPI(MetalWindow, MTLAPI)
	};
}

#endif /* __RAYNE_METALWINDOW_H__ */
