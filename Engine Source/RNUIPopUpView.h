//
//  RNUIPopUpView.h
//  Rayne
//
//  Created by Nils Daumann on 30.05.14.
//  Copyright (c) 2014 Sidney Just. All rights reserved.
//

#include "RNUIButton.h"
#include "RNUITableView.h"
#include "RNUIMenu.h"


#ifndef __Rayne__RNUIPopUpView__
#define __Rayne__RNUIPopUpView__

namespace RN
{
	namespace UI
	{
		class PopUpView : public Button, public TableViewDataSource, public TableViewDelegate
		{
		public:
			PopUpView();
			~PopUpView();
			
			RNAPI size_t TableViewNumberOfRows(TableView *tableView) override;
			RNAPI TableViewCell *TableViewCellForRow(TableView *tableView, size_t row) override;
			RNAPI void TableViewDidSelectRow(TableView *tableView, size_t row) override;
			
			RNAPI void SetMenu(Menu *menu);
			RNAPI Menu *GetMenu() const { return _menu; }
			RNAPI void SetSelection(size_t row);
			RNAPI size_t GetSelection() const { return _selected; }
			
			
		protected:
			RNAPI bool PostEvent(EventType event) override;
			
		private:
			Menu *_menu;
			Widget *_popUpWidget;
			TableView *_popUpTableView;
			size_t _selected;
		};
	}
}

#endif /* defined(__Rayne__RNUIPopUpView__) */
