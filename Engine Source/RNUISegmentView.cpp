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
		SegmentView::SegmentView() :
			_requiresSelection(false),
			_singleSelection(false)
		{}
		
		SegmentView::~SegmentView()
		{}
		
		
		
		void SegmentView::InsertSegmentAtIndex(Image *image, size_t index)
		{
			Button *segment = Button::WithType(Button::Type::Bezel);
			segment->SetImageForState(image, Control::State::Normal);
			segment->SetImagePosition(RN::UI::ImagePosition::ImageOnly);
			
			InsertSegment(segment, index);
		}
		
		void SegmentView::InsertSegmentAtIndex(String *title, size_t index)
		{
			Button *segment = Button::WithType(Button::Type::Bezel);
			segment->SetTitleForState(title, Control::State::Normal);
			segment->SetImagePosition(RN::UI::ImagePosition::NoImage);
			
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
			segment->SetImageForState(image, Control::State::Normal);
			segment->SetImagePosition(RN::UI::ImagePosition::ImageOnly);
		}
		
		void SegmentView::SetTitleForSegmentAtIndex(String *title, size_t index)
		{
			Button *segment = _segments.GetObjectAtIndex<Button>(index);
			segment->SetTitleForState(title, Control::State::Normal);
			segment->SetImagePosition(RN::UI::ImagePosition::NoImage);
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
		
		
		void SegmentView::SetRequiresSelection(bool requiresSelection)
		{
			_requiresSelection = requiresSelection;
			
			if(_requiresSelection && _segments.GetCount() > 0)
			{
				bool hasSelection = false;
				
				_segments.Enumerate<Button>([&](Button *button, size_t index, bool &stop) {
					
					if(button->IsSelected())
					{
						hasSelection = true;
						stop = true;
					}
					
				});
				
				if(!hasSelection)
					_segments.GetObjectAtIndex<Button>(0)->SetSelected(true);
			}
		}
		
		void SegmentView::SetRequiresSingleSelection(bool singleSelection)
		{
			_singleSelection = singleSelection;
			
			if(_singleSelection)
			{
				bool hasSelection = false;
				
				_segments.Enumerate<Button>([&](Button *button, size_t index, bool &stop) {
					
					if(button->IsSelected())
					{
						if(hasSelection)
							button->SetSelected(false);
						
						hasSelection = true;
					}
				});
			}
		}
		
		void SegmentView::SetFontForSegmentAtIndex(Font *font, size_t index)
		{
			if(_segments.GetCount() > index)
			{
				Button *segment = _segments.GetObjectAtIndex<Button>(index);
				segment->SetFontForState(font, Control::State::Normal);
			}
		}
		void SegmentView::SetTextColorForSegmentAtIndex(Color *color, size_t index)
		{
			Button *segment = _segments.GetObjectAtIndex<Button>(index);
			segment->SetTitleColorForState(color, Control::State::Normal);
		}
		
		
		size_t SegmentView::GetSelection() const
		{
			size_t selection = k::NotFound;
			
			_segments.Enumerate<Button>([&](Button *button, size_t index, bool &stop) {
				
				if(button->IsSelected())
				{
					selection = index;
					stop = true;
				}
				
			});
			
			return selection;
		}
		
		
		void SegmentView::InsertSegment(Button *segment, size_t index)
		{
			if(index >= _segments.GetCount() || index == k::NotFound)
			{
				_segments.AddObject(segment);
			}
			else
			{
				_segments.InsertObjectAtIndex(segment, index);
			}
			
			segment->SetBehavior(Button::Behavior::Switch);
			segment->AddListener(Control::EventType::MouseUpInside, [&](Control *control, EventType event) {
				
				if(_singleSelection)
				{
					Button *selection = control->Downcast<Button>();
					
					_segments.Enumerate<Button>([&](Button *button, size_t index, bool &stop) {
						
						if(button != selection && button->IsSelected())
							button->SetSelected(false);
					});
				}
				
				if(_requiresSelection && !control->IsSelected())
				{
					Button *selection = control->Downcast<Button>();
					bool hasSelection = false;
					
					_segments.Enumerate<Button>([&](Button *button, size_t index, bool &stop) {
						
						if(button != selection && button->IsSelected())
						{
							hasSelection = true;
							stop = true;
						}
					});
					
					if(!hasSelection)
						selection->SetSelected(true);
				}
				
				DispatchEvent(EventType::ValueChanged);
			}, this);
			
			if(_requiresSelection && _segments.GetCount() == 1)
				segment->SetSelected(true);
			
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
