//
//  RNUISegmentView.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUISegmentView.h"

namespace RN
{
	namespace UI
	{
		SegmentView::SegmentView()
		{
		}
		
		SegmentView::~SegmentView()
		{
		}
		
		
		
		void SegmentView::InsertegmentAtIndex(Image *image, size_t index)
		{
			Button *segment = Button::WithType(Button::Type::Bezel);
			segment->SetImageForState(image, Control::Normal);
			
			InsertSegment(segment, index);
		}
		
		void SegmentView::InsertSegmentAtIndex(String *title, size_t index)
		{
			Button *segment = Button::WithType(Button::Type::Bezel);
			segment->SetTitleForState(title, Control::Normal);
			
			InsertSegment(segment, index);
		}
		
		void SegmentView::RemoveSegmentAtIndex(size_t index)
		{
			Button *segment = _segments.GetObjectAtIndex<Button>(index);
			_segments.RemoveObjectAtIndex(index);
			
			segment->RemoveFromSuperview();
			SetNeedsLayoutUpdate();
		}
		
		void SegmentView::RemoveAllSegments()
		{
			_segments.Enumerate<Button>([&](Button *button, size_t index, bool &stop) {
				button->RemoveFromSuperview();
			});
			
			_segments.RemoveAllObjects();
			SetNeedsLayoutUpdate();
		}
		
		
		
		void SegmentView::SetImageForSegmentAtIndex(Image *image, size_t index)
		{
			Button *segment = _segments.GetObjectAtIndex<Button>(index);
			segment->SetImageForState(image, Control::Normal);
		}
		
		void SegmentView::SetTitleForSegmentAtIndex(String *title, size_t index)
		{
			Button *segment = _segments.GetObjectAtIndex<Button>(index);
			segment->SetTitleForState(title, Control::Normal);
		}
		
		
		
		void SegmentView::SetSegmentAtIndexEnabled(size_t index, bool enabled)
		{
			Button *segment = _segments.GetObjectAtIndex<Button>(index);
			segment->SetSelected(enabled);
		}
		
		bool SegmentView::IsSegmentAtIndexEnabled(size_t index) const
		{
			Button *segment = _segments.GetObjectAtIndex<Button>(index);
			return segment->IsSelected();
		}
		
		
		
		void SegmentView::InsertSegment(Button *segment, size_t index)
		{
			if(index == _segments.GetCount())
			{
				_segments.AddObject(segment);
			}
			else
			{
				_segments.InsertObjectAtIndex(segment, index);
			}
			
			segment->SetBehavior(Button::Behavior::Switch);
			segment->AddListener(Control::EventType::MouseUpInside, [&](Control *control, EventType event) {
				DispatchEvent(EventType::ValueChanged);
			}, this);
			
			AddSubview(segment);
			SetNeedsLayoutUpdate();
		}
		
		void SegmentView::LayoutSubviews()
		{
			size_t count = _segments.GetCount();
			
			if(count > 0)
			{
				float width  = GetFrame().width / count;
				float height = GetFrame().height;
				
				_segments.Enumerate<Button>([&](Button *button, size_t index, bool &stop) {
					Rect frame = Rect(index * width, 0, width, height);
					button->SetFrame(frame);
				});
			}
		}
	}
}
