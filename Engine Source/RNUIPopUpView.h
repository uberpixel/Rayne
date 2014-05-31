//
//  RNUIPopUpView.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBase.h"
#include "RNUIButton.h"
#include "RNUITableView.h"
#include "RNUIMenu.h"

#ifndef __RAYNE_UIPOPUPVIEW_H__
#define __RAYNE_UIPOPUPVIEW_H__

namespace RN
{
	namespace UI
	{
		class PopUpView : public Button, TableViewDataSource, TableViewDelegate
		{
		public:
			RNAPI PopUpView();
			RNAPI ~PopUpView();
			
			RNAPI void SetMenu(Menu *menu);
			RNAPI Menu *GetMenu() const { return _menu; }
			RNAPI void SetSelection(size_t row);
			RNAPI size_t GetSelection() const { return _selected; }
			
		protected:
			RNAPI bool PostEvent(EventType event) override;
			
		private:
			RNAPI size_t TableViewNumberOfRows(TableView *tableView) override;
			RNAPI TableViewCell *TableViewCellForRow(TableView *tableView, size_t row) override;
			RNAPI void TableViewDidSelectRow(TableView *tableView, size_t row) override;
			
			Menu *_menu;
			Widget *_popUpWidget;
			TableView *_popUpTableView;
			size_t _selected;
		};
	}
}

#endif /* __RAYNE_UIPOPUPVIEW_H__ */
