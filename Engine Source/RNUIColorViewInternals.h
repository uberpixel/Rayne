//
//  RNUIColorViewInternals.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UICOLORVIEWINTERNALS_H__
#define __Rayne__RNUIColorViewInternals__

#include "RNBase.h"
#include "RNUIView.h"

namespace RN
{
	namespace UI
	{
		class ColorViewContent : public View
		{
		public:
			ColorViewContent();
			
			void SetFrame(const Rect &frame) override;
			void SetColor(Color *color);
			
			void Draw(Renderer *renderer) override;
			
		private:
			Mesh *_mesh;
			Material *_material;
		};
	}
}

#endif /* __RAYNE_UICOLORVIEWINTERNALS_H__ */
