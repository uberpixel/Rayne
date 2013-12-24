//
//  RNUIScrollerInternals.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIScrollerInternals.h"

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(ScrollerKnob)
		RNDeclareMeta(ScrollerFrame)
		
		ScrollerKnob::ScrollerKnob()
		{
			SetInteractionEnabled(true);
		}
		
		void ScrollerKnob::MouseDragged(Event *event)
		{
			printf("Drag queen\n");
		}
		
		
		ScrollerFrame::ScrollerFrame()
		{
			SetInteractionEnabled(true);
		}
	}
}
