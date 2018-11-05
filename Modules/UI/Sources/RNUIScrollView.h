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
			UIAPI ScrollView();
			UIAPI ~ScrollView();

			UIAPI void Draw(Context *context) const override;
			UIAPI void Update(float delta, Vector2 cursorPosition, bool touched);
			
			bool IsScrolling() const { return _isScrolling; }

		private:
			bool _isScrollEnabled;
			bool _isScrolling;
			bool _wasTouched;
			
			float _tapTimer;
			
			float _scrollSpeed;
			Vector2 _previousCursorPosition;

			RNDeclareMetaAPI(ScrollView, UIAPI)
		};
	}
}


#endif /* __RAYNE_UISCROLLVIEW_H_ */
