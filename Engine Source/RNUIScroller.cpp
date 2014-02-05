//
//  RNUIScroller.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIScroller.h"
#include "RNUIScrollerInternals.h"
#include "RNUIScrollView.h"
#include "RNUIStyle.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Scroller)
		
		Scroller::Scroller() :
			_container(nullptr)
		{
			_frame = new ScrollerFrame();
			_frame->SetAutoresizingMask(AutoresizingFlexibleWidth | AutoresizingFlexibleHeight);
			
			_knob = new ScrollerKnob();

			AddSubview(_frame);
			AddSubview(_knob);
		}
		
		Scroller::~Scroller()
		{
			_frame->Release();
			_knob->Release();
		}
		
		void Scroller::InsertIntoContainer(ScrollView *scrollView, bool horizontal)
		{
			RN_ASSERT(_container == nullptr, "Scroller can only be added to one ScrollView at a time");
			
			_container  = scrollView;
			_horizontal = horizontal;
			
			
			Style *style = Style::GetSharedInstance();
			
			_frame->SetImage(style->ParseImage(style->GetResourceWithKeyPath<Dictionary>(RNCSTR("scroller.vertical.frame"))));
			_knob->SetImage(style->ParseImage(style->GetResourceWithKeyPath<Dictionary>(RNCSTR("scroller.vertical.knob"))));
		
			_width = _frame->GetSizeThatFits().x;
		}
		
		void Scroller::LayoutSubviews()
		{
			if(_container)
			{
				const Rect& bounds = GetBounds();
				
				const Vector2& size = _container->GetContentSize();
				const Rect& extents = _container->GetBounds();
				
				if(!_horizontal)
				{
					float offset  = (extents.y / size.y) * bounds.height;
					float visible = std::min(extents.height / size.y, 1.0f);
					float height  = std::max(bounds.height * visible, 10.0f);
					
					_knob->SetFrame(Rect(0.0f, offset, _width, height));
				}
			}
		}
	}
}
