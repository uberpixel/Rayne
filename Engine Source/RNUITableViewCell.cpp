//
//  RNUITableViewCell.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUITableViewCell.h"
#include "RNUITableView.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(TableViewCell, Control)
		
		TableViewCell::TableViewCell(String *identifier)
		{
			RN_ASSERT(identifier, "TableViewCell needs an identifier!");
			
			_identifier = identifier->Copy();
			Initialize();
		}
		
		TableViewCell::~TableViewCell()
		{
			_identifier->Release();
			
			_contentView->Release();
			_imageView->Release();
			_textLabel->Release();
		}
		
		
		
		void TableViewCell::Initialize()
		{
			_offset = 0.0f;
			_indentation = 0.0f;
			_row = 0;
			
			_contentView = new View();
			_contentView->SetBackgroundColor(Color::ClearColor());
			
			AddSubview(_contentView);
			
			_imageView = new ImageView();
			_imageView->SetScaleMode(ScaleMode::ProportionallyDown);
			
			_textLabel = new Label();
			_textLabel->SetLineBreak(LineBreakMode::TruncateMiddle);
			_textLabel->SetAutoresizingMask(AutoresizingMask::FlexibleWidth);
			
			_contentView->AddSubview(_imageView);
			_contentView->AddSubview(_textLabel);
			
			SetSelected(false);
		}
		
		void TableViewCell::PrepareForReuse()
		{
			_imageView->SetImage(nullptr);
			_textLabel->SetText(RNCSTR(""));
			_indentation = 0.0f;
			
			SetSelected(false);
			SetNeedsLayoutUpdate();
		}
		
		void TableViewCell::SetFrame(const Rect &frame)
		{
			Control::SetFrame(frame);
		}
		
		void TableViewCell::SetIndentation(float indentation)
		{
			if(Math::FastAbs(_indentation - indentation) > k::EpsilonFloat)
			{
				_indentation = indentation;
				SetNeedsLayoutUpdate();
			}
		}
		
		
		void TableViewCell::LayoutSubviews()
		{
			Control::LayoutSubviews();
			
			Rect contentFrame = Rect(Vector2(), GetFrame().GetSize());
			contentFrame.width -= _indentation;
			contentFrame.x     += _indentation;
			
			_contentView->SetFrame(contentFrame);
			
			Rect frame = Rect(Vector2(), contentFrame.GetSize()).Inset(5.0f, 2.0f);
			
			if(_imageView->GetImage())
			{
				Rect imageFrame = Rect(frame.x, frame.y, frame.height, frame.height);
				_imageView->SetFrame(imageFrame);
				
				frame.x += frame.height + 5;
				frame.width -= frame.height + 10;
			}
			
			_textLabel->SetFrame(frame);
		}
		
		void TableViewCell::SetSelected(bool selected)
		{
			Control::SetSelected(selected);
			
			if(selected)
			{
				SetBackgroundColor(Color::BlueColor());
				_textLabel->SetTextColor(Color::WhiteColor());
			}
			else
			{
				SetBackgroundColor(Color::WhiteColor());
				_textLabel->SetTextColor(Color::BlackColor());
			}
		}
		
		
		
		
		bool TableViewCell::PostEvent(EventType event)
		{
			switch(event)
			{
				case Control::EventType::MouseEntered:
				case Control::EventType::MouseLeft:
					return true;
					
				case Control::EventType::MouseDown:
					GetWidget()->MakeFirstResponder(_tableView);
					return true;
					
				case Control::EventType::MouseUpInside:
					_tableView->ConsiderCellForSelection(this);
					return true;
					
				case Control::EventType::MouseUpOutside:
					return true;
					
				default:
					break;
			}
			
			return Control::PostEvent(event);
		}
	}
}
