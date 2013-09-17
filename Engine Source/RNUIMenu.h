//
//  RNUIMenu.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIMENU_H__
#define __RAYNE_UIMENU_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNAttributedString.h"
#include "RNArray.h"
#include "RNUIImage.h"

#define kRNMenuChangedMessage RNCSTR("kRNMenuChangedMessage")
#define kRNMenuItemChangedMessage RNCSTR("kRNMenuItemChangedMessage")

namespace RN
{
	namespace UI
	{
		class MenuItem;
		class View;
		
		class Menu : public Object
		{
		public:
			Menu();
			~Menu();
			
			void AddItem(MenuItem *item);
			void InsertItem(MenuItem *item, size_t index);
			void RemoveItem(MenuItem *item);
			void RemoveItemAtIndex(size_t index);
			
			const Array *GetItems() const { return _items; }
			
		private:
			Array *_items;
			
			RNDefineMeta(Menu, Object)
		};
		
		
		class MenuItem : public Object
		{
		public:
			friend class Menu;
			
			typedef std::function<void (MenuItem *item)> Callback;
			
			MenuItem();
			MenuItem(String *title);
			MenuItem(String *title, String *keyEquivalent);
			~MenuItem();
			
			static MenuItem *WithTitle(String *title, const Callback& callback = Callback());
			static MenuItem *WithTitleAndKeyEquivalent(String *title, const Callback& callback, String *key);
			static MenuItem *SeparatorItem();
			
			void SetTitle(String *title);
			void SetAttributedTitle(AttributedString *title);
			void SetImage(Image *image);
			void SetKeyEquivalent(String *key);
			void SetCallback(const Callback& callback);
			
			void SetSubMenu(Menu *menu);
			void SetEnabled(bool enabled);
			
			Menu *GetMenu() { return _menu; }
			Menu *GetSubMenu() { return _subMenu; }
			
			bool IsEnabled() const;
			
			const String *GetTitle() const { return _title->GetString(); }
			const AttributedString *GetAttributedTitle() const { return _title; }
			const Image *GetImage() const { return _image; }
			const String *GetKeyEquivalent() const { return _keyEquivalent; }
			const Callback& GetCallback() const { return _callback; }
			
			bool IsSeparator() const { return _isSeparator; }
			
		private:
			void RemoveFromMenu();
			
			bool _isSeparator;
			bool _isEnabled;
			
			AttributedString *_title;
			Image *_image;
			String *_keyEquivalent;
			
			Callback _callback;
			
			Menu *_menu;
			Menu *_subMenu;
			
			RNDefineMeta(MenuItem, Object)
		};
	}
}

#endif /* __RAYNE_UIMENU_H__ */
