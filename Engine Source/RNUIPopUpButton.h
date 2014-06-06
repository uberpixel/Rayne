//
//  RNUIPopUpButton.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBase.h"
#include "RNUIButton.h"
#include "RNUITableView.h"
#include "RNUIMenu.h"

#ifndef __RAYNE_UIPOPUPBUTTON_H__
#define __RAYNE_UIPOPUPBUTTON_H__

namespace RN
{
	namespace UI
	{
		class PopUpButton : public Button, TableViewDataSource, TableViewDelegate
		{
		public:
			RNAPI PopUpButton();
			RNAPI ~PopUpButton();
			
			RNAPI void SetMenu(Menu *menu);
			RNAPI void SetSelection(size_t index);
			
			RNAPI Menu *GetMenu() const { return _menu; }
			RNAPI MenuItem *GetSelectedItem() const;
			RNAPI size_t GetSelection() const { return _popUpTableView->GetSelection()->GetFirstIndex(); }
			
		protected:
			RNAPI bool PostEvent(EventType event) override;
			
		private:
			RNAPI size_t TableViewNumberOfRows(TableView *tableView) override;
			RNAPI TableViewCell *TableViewCellForRow(TableView *tableView, size_t row) override;
			RNAPI void TableViewDidSelectRow(TableView *tableView, size_t row) override;
			
			Menu *_menu;
			Widget *_popUpWidget;
			TableView *_popUpTableView;
		};
	}
}

#endif /* __RAYNE_UIPOPUPBUTTON_H__ */
