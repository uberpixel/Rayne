//
//  RNUIScrollView.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIScrollView.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(ScrollView, View)
		
		ScrollView::ScrollView() :
			_verticalScroller(nullptr),
			_delegate(nullptr)
		{
			SetClipSubviews(true);
			SetVerticalScroller((new Scroller())->Autorelease());
		}
		
		ScrollView::~ScrollView()
		{
			if(_verticalScroller)
			{
				_verticalScroller->_container = nullptr;
				_verticalScroller->Release();
			}
		}
		
		
		void ScrollView::CalculateContentSize()
		{
			_size = Vector2();
			
			GetSubivews()->Enumerate([&](Object *tview, size_t index, bool &stop) {
				View *view = tview->Downcast<View>();
				
				_size.x = std::max(view->GetFrame().GetRight(),  _size.x);
				_size.y = std::max(view->GetFrame().GetBottom(), _size.y);
			});
			
			SetContentSize(_size);
		}
		
		void ScrollView::SetFrame(const Rect& frame)
		{
			View::SetFrame(frame);
			SetContentSize(_size);
		}
		
		void ScrollView::SetDelegate(ScrollViewDelegate *delegate)
		{
			_delegate = delegate;
		}
		
		
		
		void ScrollView::SetContentOffset(const Vector2& offset)
		{
			Rect bounds = GetBounds();
			bounds.x = offset.x;
			bounds.y = offset.y;
			
			SetBounds(bounds);
			
			_offset = offset;
			
			if(_delegate)
				_delegate->ScrollViewDidScroll(this);
			
			AdjustScroller();
		}
		void ScrollView::SetContentSize(const Vector2& size)
		{
			_size = size;
			_end  = _size - GetFrame().GetSize();
			
			_end.x = std::max(0.0f, _end.x);
			_end.y = std::max(0.0f, _end.y);
			
			if(_offset.y > _end.y || _offset.x > _end.x)
			{
				_offset.x = std::min(_offset.x, _end.x);
				_offset.y = std::min(_offset.y, _end.y);
				
				SetContentOffset(_offset);
				return;
			}
			
			AdjustScroller();
		}
		
		
		
		void ScrollView::SetVerticalScroller(Scroller *scroller)
		{
			if(_verticalScroller)
			{
				_verticalScroller->_container = nullptr;
				_verticalScroller->Release();
				_verticalScroller = nullptr;
			}
			
			_verticalScroller = SafeRetain(scroller);
			
			if(_verticalScroller)
			{
				_verticalScroller->InsertIntoContainer(this, false);
				
				AddSubview(_verticalScroller);
				AdjustScroller();
			}
		}
		
		
		void ScrollView::AdjustScroller()
		{
			if(_verticalScroller)
			{
				if(_size.y > GetBounds().height)
				{
					const Rect& bounds = GetBounds();
					float width = _verticalScroller->GetPreferredWidth();
				
					_verticalScroller->SetFrame(Rect(bounds.width - width, bounds.y, width, bounds.height));
					_verticalScroller->SetHidden(false);
				}
				else
				{
					_verticalScroller->SetHidden(true);
				}
			}
		}
		
		void ScrollView::DidAddSubview(View *subview)
		{
			if(subview != _verticalScroller)
			{
				BringSubviewToFront(_verticalScroller);
			}
		}
		
		void ScrollView::DidBringSubviewToFront(View *subview)
		{
			if(subview != _verticalScroller)
			{
				BringSubviewToFront(_verticalScroller);
			}
		}
		
		
		void ScrollView::ScrollWheel(Event *event)
		{
			Vector2 delta = -event->GetMouseWheel();
			delta += _offset;
			
			delta.x = std::max(0.0f, delta.x);
			delta.y = std::max(0.0f, delta.y);
			
			delta.x = delta.x > _end.x ? _end.x : delta.x;
			delta.y = delta.y > _end.y ? _end.y : delta.y;
			
			SetContentOffset(delta);
		}
		
		void ScrollView::MoveScroller(Vector2 delta)
		{
			delta /= GetFrame().GetSize();
			delta *= _size;
			delta += _offset;
			
			delta.x = std::max(0.0f, delta.x);
			delta.y = std::max(0.0f, delta.y);
			
			delta.x = delta.x > _end.x ? _end.x : delta.x;
			delta.y = delta.y > _end.y ? _end.y : delta.y;
			
			SetContentOffset(delta);
		}
	}
}
