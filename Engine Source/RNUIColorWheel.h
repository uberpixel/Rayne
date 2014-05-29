//
//  RNUIColorWheel.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UICOLORWHEEL_H__
#define __RAYNE_UICOLORWHEEL_H__

#include "RNBase.h"
#include "RNUIColor.h"
#include "RNUIView.h"

namespace RN
{
	namespace UI
	{
		class ColorWheel : public View
		{
		public:
			RNAPI ColorWheel();
			
			RNAPI void Update() override;
			RNAPI void Draw(Renderer *renderer) override;
			
		private:
			void Initialize();
			
			Mesh *_mesh;
			Material *_material;
			
			bool _isDirty;
			
			RNDeclareMeta(ColorWheel)
		};
	}
}

#endif /* __RAYNE_UICOLORWHEEL_H__ */
