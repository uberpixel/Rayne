//
//  RNUITableViewCell.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUITableViewCell.h"
#include "RNUITableView.h"

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(TableViewCell)
		
		TableViewCell::TableViewCell(String *identifier)
		{
			RN_ASSERT(identifier, "TableViewCell needs an identifier!");
			
			_identifier = identifier->Retain();
			Initialize();
		}
		
		TableViewCell::~TableViewCell()
		{
			_identifier->Release();
			
			_imageView->Release();
			_textLabel->Release();
		}
		
		
		
		void TableViewCell::Initialize()
		{
			_offset = 0.0f;
			_row = 0;
			
			_imageView = new ImageView();
			_textLabel = new Label();
			
			_imageView->SetFrame(Rect(Vector2(), Vector2(15.0f)));
			
			AddSubview(_imageView);
			AddSubview(_textLabel);
		}
		
		void TableViewCell::PrepareForReuse()
		{
			_imageView->SetImage(nullptr);
			_textLabel->SetText(RNCSTR(""));
			
			SetSelected(false);
		}
		
		void TableViewCell::LayoutSubviews()
		{
			Control::LayoutSubviews();
			
			Rect ownFrame = Frame();
			Rect frame = Rect(Vector2(), ownFrame.Size()).Inset(5.0f, 0.0f);
			
			if(_imageView->GetImage())
			{
				Rect imageFrame = Rect(frame.x, frame.y, frame.height, frame.height);
				_imageView->SetFrame(imageFrame);
				
				frame.x += frame.height + 5;
				frame.width -= frame.height + 5;
			}
			
			_textLabel->SetFrame(frame);
		}
		
		void TableViewCell::SetSelected(bool selected)
		{
			Control::SetSelected(selected);
			
			if(selected)
			{
				
			}
			else
			{
				
			}
		}
	}
}
