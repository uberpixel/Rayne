//
//  RNUIScrollView.h
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UISCROLLVIEW_H_
#define __RAYNE_UISCROLLVIEW_H_

#include "RNUIView.h"

namespace RN
{
	namespace UI
	{
		class ScrollView : public View
		{
		public:
			UIAPI ScrollView(bool vertical = true, bool horizontal = false);
			UIAPI ~ScrollView();

			UIAPI void Update(float delta, Vector2 cursorPosition, bool touched, Vector2 alternativeScrollSpeed = Vector2());
			
			UIAPI void SetPixelPerInch(float pixelPerInch);
			
			bool IsScrolling() const { return _isScrolling; }

		private:
			bool _isScrollEnabled;
			bool _isScrolling;
			bool _wasTouched;
			
			bool _scrollsVertical;
			bool _scrollsHorizontal;
			
			float _tapTimer;
			float _pixelPerInch;
			
			Vector2 _scrollSpeed;
			Vector2 _previousCursorPosition;

			RNDeclareMetaAPI(ScrollView, UIAPI)
		};
	}
}


#endif /* __RAYNE_UISCROLLVIEW_H_ */
