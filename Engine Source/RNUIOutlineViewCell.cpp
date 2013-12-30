//
//  RNUIOutlineViewCell.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIOutlineViewCell.h"
#include "RNUIOutlineView.h"

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(OutlineViewCell)
		
		OutlineViewCell::OutlineViewCell(String *identifier) :
			TableViewCell(identifier)
		{
			Initialize();
		}
		
		OutlineViewCell::~OutlineViewCell()
		{
			_disclosureTriangle->Release();
		}
		
		
		void OutlineViewCell::Initialize()
		{
			_item = nullptr;
			
			_disclosureTriangle = Button::WithType(Button::Type::DisclosureTriangle)->Retain();
			_disclosureTriangle->SetImagePosition(ImagePosition::ImageOnly);
			_disclosureTriangle->SizeToFit();
			_disclosureTriangle->AddListener(Control::EventType::MouseUpInside, [this](Control * control, EventType tyoe) {
				DisclosureTriangleClicked();
			}, this);
			
			AddSubview(_disclosureTriangle);
		}
		
		
		void OutlineViewCell::PrepareForReuse()
		{
			TableViewCell::PrepareForReuse();
			_expandable = false;
			_expanded = false;
		}
		
		void OutlineViewCell::LayoutSubviews()
		{
			TableViewCell::LayoutSubviews();
			
			View *contentView     = GetContentView();
			float disclosureWidth = _disclosureTriangle->GetSizeThatFits().x;
			
			Rect contentFrame = contentView->GetFrame();
			contentFrame.x += 5.0f;
			contentFrame.width -= 5.0f;
			
			// Update the content view's frame
			Rect nContentFrame = contentFrame;
			nContentFrame.x     += disclosureWidth;
			nContentFrame.width -= disclosureWidth;
			
			contentView->SetFrame(nContentFrame);
			
			// Center the disclosure triangle vertically
			Rect frame = GetFrame();
			Rect disclosureFrame = contentFrame;
			
			disclosureFrame.width  = _disclosureTriangle->GetFrame().width;
			disclosureFrame.height = _disclosureTriangle->GetFrame().height;
			
			disclosureFrame.y = (frame.height / 2.0f) - (disclosureFrame.height / 2.0f);
			
			_disclosureTriangle->SetFrame(disclosureFrame);
			_disclosureTriangle->SetHidden(!_expandable);
			_disclosureTriangle->SetSelected(_expanded);
		}
		
		
		void OutlineViewCell::DisclosureTriangleClicked()
		{
			if(_disclosureTriangle->IsSelected())
			{
				_outlineView->ExpandItem(_item, false);
			}
			else
			{
				_outlineView->CollapseItem(_item, false);
			}
		}
	}
}
