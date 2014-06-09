//
//  RNUISlider.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UISLIDER_H__
#define __RAYNE_UISLIDER_H__

#include "RNBase.h"
#include "RNUIControl.h"
#include "RNUIImageView.h"

namespace RN
{
	namespace UI
	{
		class Slider : public Control
		{
		public:
			enum class Direction
			{
				Vertical,
				Horizontal
			};
			
			RNAPI Slider(Direction direction);
			RNAPI Slider(Dictionary *style, Direction direction);
			RNAPI ~Slider();
			
			RNAPI void SetMinValue(float minValue);
			RNAPI void SetMaxValue(float maxValue);
			RNAPI void SetValue(float value);
			
			RNAPI void MouseDown(Event *event) override;
			RNAPI void MouseDragged(Event *event) override;
			
			RNAPI float GetValue() const { return _value; }
			
			RNAPI void LayoutSubviews() override;
			
		protected:
			RNAPI void StateChanged(State state) override;
			
		private:
			void HandleEvent(Event *event);
			void LayoutKnob();
			
			Direction _direction;
			
			float _min;
			float _max;
			float _value;
			
			ImageView *_background;
			ImageView *_knob;
			
			ControlStateStore<Image> _knobImages;
			
			RNDeclareMeta(Slider)
		};
	}
}

#endif /* __RAYNE_UISLIDER_H__ */
