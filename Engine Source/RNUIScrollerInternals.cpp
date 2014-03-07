//
//  RNUIScrollerInternals.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIScrollerInternals.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(ScrollerKnob)
		RNDefineMeta(ScrollerFrame)
		
		ScrollerKnob::ScrollerKnob()
		: _scrollView(nullptr)
		{
			SetInteractionEnabled(true);
		}
		
		void ScrollerKnob::SetScrollView(ScrollView *view)
		{
			_scrollView = view;
		}
		
		void ScrollerKnob::MouseDragged(Event *event)
		{
			if(_scrollView)
			{
				_scrollView->MoveScroller(-event->GetMouseDelta());
			}
		}
		
		
		ScrollerFrame::ScrollerFrame()
		{
			SetInteractionEnabled(true);
		}
	}
}
