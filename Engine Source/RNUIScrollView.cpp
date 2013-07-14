//
//  RNUIScrollView.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIScrollView.h"

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(ScrollView)
		
		ScrollView::ScrollView()
		{
			SetClipSubviews(true);
		}
		
		ScrollView::~ScrollView()
		{
		}
		
		
		void ScrollView::CalculateContentSize()
		{
		}
		
		
		void ScrollView::SetContentOffset(const Vector2& offset)
		{
			Rect bounds = Bounds();
			bounds.x = offset.x;
			bounds.y = offset.y;
			
			SetBounds(bounds);
			
			_offset = offset;
		}
		void ScrollView::SetContentSize(const Vector2& size)
		{
			_size = size;
		}
	}
}
