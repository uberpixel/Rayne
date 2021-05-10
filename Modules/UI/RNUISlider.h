//
//  RNUISlider.h
//  Rayne
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#ifndef __RAYNE_UISLIDER_H_
#define __RAYNE_UISLIDER_H_

#include "RNUIView.h"

namespace RN
{
	namespace UI
	{
		class Slider : public View
		{
		public:
			UIAPI Slider(const Rect &frame, float value, float from, float to, float step);
			UIAPI ~Slider();

			UIAPI void Update(float delta, Vector2 cursorPosition, bool touched);
			float GetValue() const { return _value; }

		private:
			float _value;
			float _from;
			float _to;
			float _step;
			
			View *_handleView;

			RNDeclareMetaAPI(Slider, UIAPI)
		};
	}
}


#endif /* __RAYNE_UISLIDER_H_ */
