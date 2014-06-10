//
//  RNUIPopUpButton.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIPopUpButton.h"

namespace RN
{
	namespace UI
	{
		PopUpButton::PopUpButton() :
			Button(Button::Type::Bezel),
			_menu(nullptr)
		{
			_popUpTableView = new TableView();
			_popUpTableView->SetDataSource(this);
			_popUpTableView->SetDelegate(this);
			
			_popUpWidget = new Widget(Widget::Style::Borderless);
			_popUpWidget->SetContentView(_popUpTableView);
			_popUpWidget->SetWidgetLevel(kRNUIWidgetLevelFloating);
			_popUpWidget->SetDelegate(this);
		}
		
		PopUpButton::~PopUpButton()
		{
			SafeRelease(_popUpWidget);
			SafeRelease(_popUpTableView);
			SafeRelease(_menu);
		}
		
		bool PopUpButton::PostEvent(EventType event)
		{
			if(!_popUpWidget->IsOpen())
			{
				if(event == EventType::MouseUpInside)
				{
					Vector2 posTop = ConvertPointToBase(Vector2(GetBounds().x, GetBounds().y));
					Vector2 posBottom = ConvertPointToBase(Vector2(GetBounds().x, GetBounds().y + GetBounds().height));
					float height = _popUpTableView->GetContentSize().y;
					Vector2 windowSize = Window::GetSharedInstance()->GetSize();
					
					Rect frame(posBottom, GetBounds().width, height);
					
					if(posBottom.y + height > windowSize.y)
					{
						//more space to the top
						if((windowSize.y - posBottom.y) < posTop.y)
						{
							if(posTop.y < height)
							{
								height = posTop.y;
								frame.height = height;
							}
							
							frame.y = posTop.y - height;
						}
						else
						{
							height = windowSize.y - posBottom.y;
							frame.height = height;
						}
					}
					
					_popUpWidget->SetFrame(frame);
					
					_popUpWidget->Open();
					_popUpWidget->MakeKeyWidget();
				}
			}
			else
			{
				if(event == EventType::MouseUpInside || event == EventType::MouseUpOutside)
				{
					RN::Vector2 mousePosition = RN::Input::GetSharedInstance()->GetMousePosition();
					Rect frame = _popUpWidget->GetFrame();
					if(!frame.ContainsPoint(mousePosition))
					{
						_popUpWidget->Close();
					}
				}
			}
			
			return Button::PostEvent(event);
		}
		
		void PopUpButton::WidgetDidResignKey(Widget *widget)
		{
			RN::Vector2 mousePosition = RN::Input::GetSharedInstance()->GetMousePosition();
			mousePosition = ConvertPointFromBase(mousePosition);
			if(!GetBounds().ContainsPoint(mousePosition))
				_popUpWidget->Close();
		}
		
		size_t PopUpButton::TableViewNumberOfRows(TableView *tableView)
		{
			if(!_menu)
				return 0;
			
			return _menu->GetItems()->GetCount();
		}
		
		TableViewCell *PopUpButton::TableViewCellForRow(TableView *tableView, size_t row)
		{
			TableViewCell *cell = tableView->DequeueCellWithIdentifier(RNCSTR("Cell"));
			
			if(!cell)
			{
				cell = new TableViewCell(RNCSTR("Cell"));
				cell->Autorelease();
			}
			
			MenuItem *item = _menu->GetItems()->GetObjectAtIndex<MenuItem>(row);
			
			cell->GetTextLabel()->SetAttributedText(const_cast<AttributedString *>(item->GetAttributedTitle()));
			cell->SetBackgroundColor(Color::WhiteColor());
			cell->GetTextLabel()->SetTextColor(Color::BlackColor());
			cell->GetImageView()->SetImage(const_cast<Image *>(item->GetImage()));
			
			return cell;
		}
		
		void PopUpButton::TableViewDidSelectRow(TableView *tableView, size_t row)
		{
			MenuItem *item = _menu->GetItems()->GetObjectAtIndex<MenuItem>(row);
			SetTitleForState(const_cast<String *>(item->GetTitle()), Control::State::Normal);
			DispatchEvent(EventType::ValueChanged);
			
			_popUpWidget->Close();
		}
		
		void PopUpButton::SetMenu(RN::UI::Menu *menu)
		{
			SafeRelease(_menu);
			_menu = menu->Retain();
			
			_popUpTableView->ReloadData();
		}
		
		void PopUpButton::SetSelection(size_t index)
		{
			if(!_menu)
				return;
			
			_popUpTableView->SetSelection((new RN::IndexSet(index))->Autorelease());
		}
		
		MenuItem *PopUpButton::GetSelectedItem() const
		{
			if(!_menu)
				return nullptr;
			
			return _menu->GetItems()->GetObjectAtIndex<MenuItem>(_popUpTableView->GetSelection()->GetFirstIndex());
		}
	}
}
