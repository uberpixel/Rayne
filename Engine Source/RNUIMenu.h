//
//  RNUIMenu.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
			RNAPI Menu();
			RNAPI ~Menu();
			
			RNAPI void AddItem(MenuItem *item);
			RNAPI void InsertItem(MenuItem *item, size_t index);
			RNAPI void RemoveItem(MenuItem *item);
			RNAPI void RemoveItemAtIndex(size_t index);
			
			RNAPI const Array *GetItems() const { return _items; }
			
			RNAPI static void PopUpContextMenu(Menu *menu, const Vector2& location);
			
		private:
			Array *_items;
			
			RNDeclareMeta(Menu)
		};
		
		
		class MenuItem : public Object
		{
		public:
			friend class Menu;
			
			typedef std::function<void (MenuItem *item)> Callback;
			
			RNAPI MenuItem();
			RNAPI MenuItem(String *title);
			RNAPI MenuItem(String *title, String *keyEquivalent);
			RNAPI ~MenuItem();
			
			RNAPI static MenuItem *WithTitle(String *title, const Callback& callback = Callback());
			RNAPI static MenuItem *WithTitleAndKeyEquivalent(String *title, const Callback& callback, String *key);
			RNAPI static MenuItem *SeparatorItem();
			
			RNAPI void SetTitle(String *title);
			RNAPI void SetAttributedTitle(AttributedString *title);
			RNAPI void SetImage(Image *image);
			RNAPI void SetKeyEquivalent(String *key);
			RNAPI void SetKeyEquivalentModifierMask(uint32 mask);
			RNAPI void SetCallback(const Callback& callback);
			
			RNAPI void SetSubMenu(Menu *menu);
			RNAPI void SetEnabled(bool enabled);
			
			RNAPI Menu *GetMenu() { return _menu; }
			RNAPI Menu *GetSubMenu() { return _subMenu; }
			
			RNAPI bool IsEnabled() const;
			
			RNAPI const String *GetTitle() const { return _title->GetString(); }
			RNAPI const AttributedString *GetAttributedTitle() const { return _title; }
			RNAPI const Image *GetImage() const { return _image; }
			RNAPI const String *GetKeyEquivalent() const { return _keyEquivalent; }
			RNAPI uint32 GetKeyEquivalentModifierMask() const { return _keyEquivalentModifierMask; }
			RNAPI const Callback& GetCallback() const { return _callback; }
			
			RNAPI bool IsSeparator() const { return _isSeparator; }
			
		private:
			void RemoveFromMenu();
			
			bool _isSeparator;
			bool _isEnabled;
			
			AttributedString *_title;
			Image *_image;
			String *_keyEquivalent;
			uint32 _keyEquivalentModifierMask;
			
			Callback _callback;
			
			Menu *_menu;
			Menu *_subMenu;
			
			RNDeclareMeta(MenuItem)
		};
	}
}

#endif /* __RAYNE_UIMENU_H__ */
