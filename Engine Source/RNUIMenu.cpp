//
//  RNUIMenu.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIMenu.h"
#include "RNUIView.h"
#include "RNMessage.h"

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(Menu)
		RNDeclareMeta(MenuItem)
		
		// ---------------------
		// MARK: -
		// MARK: Menu
		// ---------------------
		
		Menu::Menu()
		{
			_items = new Array();
		}
		
		Menu::~Menu()
		{
			_items->Release();
		}
		
		void Menu::AddItem(MenuItem *item)
		{
			InsertItem(item, _items->GetCount());
		}
		
		void Menu::InsertItem(MenuItem *item, size_t index)
		{
			item->Retain();
			item->RemoveFromMenu();
			
			item->_menu = this;
			
			if(index == _items->GetCount())
				_items->AddObject(item);
			else
				_items->InsertObjectAtIndex(item, index);
			
			MessageCenter::GetSharedInstance()->PostMessage(kRNMenuChangedMessage, this, nullptr);
			item->Release();
		}
		
		void Menu::RemoveItem(MenuItem *item)
		{
			size_t index = _items->GetIndexOfObject(item);
			RemoveItemAtIndex(index);
		}
		
		void Menu::RemoveItemAtIndex(size_t index)
		{
			MenuItem *item = _items->GetObjectAtIndex<MenuItem>(index);
			item->_menu = nullptr;
			
			_items->RemoveObjectAtIndex(index);
			
			MessageCenter::GetSharedInstance()->PostMessage(kRNMenuChangedMessage, this, nullptr);
		}
		
		// ---------------------
		// MARK: -
		// MARK: Menu Item
		// ---------------------
		
		MenuItem::MenuItem()
		{
			_menu    = nullptr;
			_subMenu = nullptr;
			
			_title = nullptr;
			_image = nullptr;
			
			_isEnabled     = true;
			_isSeparator   = false;
			_keyEquivalent = nullptr;
			
			SetTitle(RNCSTR(""));
			SetKeyEquivalent(RNCSTR(""));
		}
		
		MenuItem::MenuItem(String *title) :
			MenuItem()
		{
			SetTitle(title);
		}
		
		MenuItem::MenuItem(String *title, String *keyEquivalent) :
			MenuItem()
		{
			SetTitle(title);
			SetKeyEquivalent(keyEquivalent);
		}
		
		MenuItem::~MenuItem()
		{
			SafeRelease(_subMenu);
			SafeRelease(_title);
			SafeRelease(_image);
		}
		
		MenuItem *MenuItem::WithTitle(String *title, const Callback& callback)
		{
			MenuItem *item = new MenuItem(title);
			item->SetCallback(callback);
			return item->Autorelease();
		}
		
		MenuItem *MenuItem::WithTitleAndKeyEquivalent(String *title, const Callback& callback, String *key)
		{
			MenuItem *item = new MenuItem(title, key);
			item->SetCallback(callback);
			return item->Autorelease();
		}
		
		MenuItem *MenuItem::SeparatorItem()
		{
			MenuItem *item = new MenuItem();
			item->_isSeparator = true;

			return item->Autorelease();
		}
		
		void MenuItem::SetKeyEquivalent(String *key)
		{
			SafeRelease(_keyEquivalent);
			_keyEquivalent = key ? key->Copy() : RNCSTR("")->Retain();
			
			MessageCenter::GetSharedInstance()->PostMessage(kRNMenuItemChangedMessage, this, nullptr);
		}
		
		void MenuItem::SetTitle(String *title)
		{
			AttributedString *string = new AttributedString(title);
			SetAttributedTitle(string);
			string->Release();
		}
		
		void MenuItem::SetAttributedTitle(AttributedString *title)
		{
			SafeRelease(_title);
			_title = SafeRetain(title);
			
			MessageCenter::GetSharedInstance()->PostMessage(kRNMenuItemChangedMessage, this, nullptr);
		}
		
		void MenuItem::SetImage(Image *image)
		{
			SafeRelease(_image);
			_image = SafeRetain(image);
			
			MessageCenter::GetSharedInstance()->PostMessage(kRNMenuItemChangedMessage, this, nullptr);
		}
		
		void MenuItem::SetEnabled(bool enabled)
		{
			_isEnabled = enabled;
			MessageCenter::GetSharedInstance()->PostMessage(kRNMenuItemChangedMessage, this, nullptr);
		}
		
		void MenuItem::SetCallback(const Callback& callback)
		{
			_callback = callback;
			MessageCenter::GetSharedInstance()->PostMessage(kRNMenuItemChangedMessage, this, nullptr);
		}
		
		void MenuItem::RemoveFromMenu()
		{
			if(_menu)
				_menu->RemoveItem(this);
		}
		
		void MenuItem::SetSubMenu(Menu *menu)
		{
			SafeRelease(_subMenu);
			_subMenu = SafeRetain(menu);
			
			MessageCenter::GetSharedInstance()->PostMessage(kRNMenuItemChangedMessage, this, nullptr);
		}
		
		bool MenuItem::IsEnabled() const
		{
			return (_isEnabled && _callback);
		}
	}
}
