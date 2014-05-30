//
//  RNUIColorPicker.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UICOLORPICKER_H__
#define __RAYNE_UICOLORPICKER_H__

#include "RNBase.h"
#include "RNUIColor.h"
#include "RNUIColorWheel.h"
#include "RNUIControl.h"

namespace RN
{
	namespace UI
	{
		class ColorPicker : public Control
		{
		public:
			RNAPI ColorPicker();
			RNAPI ColorPicker(const Rect &frame);
			RNAPI ~ColorPicker() override;
			
			RNAPI void SetColor(Color *color);
			
			RNAPI void MouseDown(Event *event) override;
			RNAPI void MouseDragged(Event *event) override;
			
			RNAPI void LayoutSubviews() override;
			
		private:
			void UpdateKnob(const Vector2 &position);
			
			Vector3 ColorFromHSV(float h, float s, float v);
			Vector3 ColorToHSV(const Vector3 &color);
			Color *ConvertColorFromWheel(const Vector2 &position, float brightness);
			Vector2 ConvertColorToWheel(Color *color, float &brightness);
			
			ColorWheel *_colorWheel;
			View *_colorKnob;
			Color *_color;
			
			RNDeclareMeta(ColorPicker)
		};
	}
}

#endif /* __RAYNE_UICOLORPICKER_H__ */
