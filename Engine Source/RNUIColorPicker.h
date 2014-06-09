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
#include "RNUIGradientView.h"
#include "RNUISlider.h"

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
			
			RNAPI void SetColor(const RN::Color &color);
			RNAPI const RN::Color &GetColor() const { return _color; }
			
			RNAPI void MouseDown(Event *event) override;
			RNAPI void MouseDragged(Event *event) override;
			
			RNAPI void LayoutSubviews() override;
			
		private:
			void UpdateWithMouseLocation(const Vector2 &point);
			
			void UpdateColorKnob(const Vector2 &position, bool updateColor);
			void UpdateColor(const RN::Color &color);
			void UpdateBrightness();
			
			RN::Color ConvertColorFromWheel(const Vector2 &position, float brightness);
			Vector2 ConvertColorToWheel(const RN::Color &color, float &brightness);
			
			ColorWheel *_colorWheel;
			GradientView *_brightnessView;
			Slider *_alphaSlider;
			
			View  *_colorKnob;
			View  *_brightnessKnob;
			View  *_hit;
			
			RN::Color _fullColor;
			RN::Color _color;
			
			float _brightness;
			
			RNDeclareMeta(ColorPicker)
		};
	}
}

#endif /* __RAYNE_UICOLORPICKER_H__ */
