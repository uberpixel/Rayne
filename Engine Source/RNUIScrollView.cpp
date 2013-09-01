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
			_delegate = nullptr;
		}
		
		ScrollView::~ScrollView()
		{
		}
		
		
		void ScrollView::CalculateContentSize()
		{
			_size = Vector2();
			
			Subivews()->Enumerate([&](Object *tview, size_t index, bool *stop) {
				View *view = tview->Downcast<View>();
				
				_size.x = std::max(view->Frame().GetRight(), _size.x);
				_size.y = std::max(view->Frame().GetBottom(),   _size.y);
			});
			
			SetContentSize(_size);
		}
		
		void ScrollView::SetFrame(const Rect& frame)
		{
			View::SetFrame(frame);
			SetContentSize(_size);
		}
		
		
		
		void ScrollView::SetContentOffset(const Vector2& offset)
		{
			Rect bounds = Bounds();
			bounds.x = offset.x;
			bounds.y = offset.y;
			
			SetBounds(bounds);
			
			_offset = offset;
			
			if(_delegate)
				_delegate->ScrollViewDidScroll(this);
		}
		void ScrollView::SetContentSize(const Vector2& size)
		{
			_size = size;
			_end  = _size - Frame().Size();
			
			_end.x = std::max(0.0f, _end.x);
			_end.y = std::max(0.0f, _end.y);
		}
		
		
		
		void ScrollView::ScrollWheel(Event *event)
		{
			Vector2 delta = event->GetMouseWheel();
			delta.x = -delta.x;
			delta += _offset;
			
			delta.x = std::max(0.0f, delta.x);
			delta.y = std::max(0.0f, delta.y);
			
			delta.x = delta.x > _end.x ? _end.x : delta.x;
			delta.y = delta.y > _end.y ? _end.y : delta.y;
			
			SetContentOffset(delta);
		}
	}
}
