//
//  RNUIScrollerInternals.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UISCROLLERINTERNALS_H__
#define __RAYNE_UISCROLLERINTERNALS_H__

#include "RNBase.h"
#include "RNUIView.h"
#include "RNUIImageView.h"
#include "RNUIScrollView.h"

namespace RN
{
	namespace UI
	{
		class ScrollerKnob : public ImageView
		{
		public:
			ScrollerKnob();
			void SetScrollView(ScrollView *view);
			
			void MouseDragged(Event *event) override;
			
			RNDeclareMeta(ScrollerKnob, ImageView)
			
		private:
			ScrollView *_scrollView;
		};
		
		class ScrollerFrame : public ImageView
		{
		public:
			ScrollerFrame();
			
			RNDeclareMeta(ScrollerFrame, ImageView)
		};
	}
}

#endif /* __RAYNE_UISCROLLERINTERNALS_H__ */
