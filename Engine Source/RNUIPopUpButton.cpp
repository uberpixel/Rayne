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
			_menu(nullptr),
			_selected(0)
		{
			_popUpTableView = new TableView();
			_popUpTableView->SetDataSource(this);
			_popUpTableView->SetDelegate(this);
			
			_popUpWidget = new Widget(Widget::Style::Borderless);
			_popUpWidget->SetContentView(_popUpTableView);
			_popUpWidget->SetWidgetLevel(kRNUIWidgetLevelFloating);
		}
		
		PopUpButton::~PopUpButton()
		{
			SafeRelease(_popUpWidget);
			SafeRelease(_popUpTableView);
			SafeRelease(_menu);
		}
		
		bool PopUpButton::PostEvent(EventType event)
		{
			if(event == EventType::MouseUpInside)
			{
				Vector2 pos = ConvertPointToBase(Vector2(GetBounds().x, GetBounds().y + GetBounds().height));
				Rect frame(pos, GetBounds().width, 100.0f);
				_popUpWidget->SetFrame(frame);
				
				_popUpWidget->Open();
			}
			
			return Button::PostEvent(event);
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
			cell->GetTextLabel()->SetTextColor(RN::Color::Black());
			cell->GetImageView()->SetImage(const_cast<Image *>(item->GetImage()));
			
			cell->SetBackgroundColor(RN::Color::White());
			cell->SetSelected(row == _selected);
			
			return cell;
		}
		
		void PopUpButton::TableViewDidSelectRow(TableView *tableView, size_t row)
		{
			_popUpWidget->Close();
			SetSelection(row);
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
			
			_popUpTableView->GetCellForRow(_selected)->SetSelected(false);
			_selected = index;
			
			MenuItem *item = _menu->GetItems()->GetObjectAtIndex<MenuItem>(_selected);
			
			SetTitleForState(const_cast<String *>(item->GetTitle()), Control::State::Normal);
			DispatchEvent(EventType::ValueChanged);
		}
		
		MenuItem *PopUpButton::GetSelectedItem() const
		{
			if(!_menu)
				return nullptr;
			
			return _menu->GetItems()->GetObjectAtIndex<MenuItem>(_selected);
		}
	}
}
