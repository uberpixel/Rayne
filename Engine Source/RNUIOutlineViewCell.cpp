//
//  RNUIOutlineViewCell.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
			
			View *contentView = GetContentView();
			float disclosureWidth = _disclosureTriangle->Frame().width;
			
			Rect contentFrame = contentView->Frame();
			Rect disclosureFrame = contentFrame;
			
			// Update the content view's frame
			contentFrame.x += disclosureWidth;
			contentFrame.width -= disclosureWidth;
			
			contentView->SetFrame(contentFrame);
			
			// Center the disclosure triangle vertically
			Rect frame = Frame();
			
			disclosureFrame.width = _disclosureTriangle->Frame().width;
			disclosureFrame.height = _disclosureTriangle->Frame().height;
			
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
