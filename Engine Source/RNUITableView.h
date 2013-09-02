//
//  RNUITableView.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UITABLEVIEW_H__
#define __RAYNE_UITABLEVIEW_H__

#include "RNBase.h"
#include "RNString.h"
#include "RNArray.h"
#include "RNDictionary.h"
#include "RNUIScrollView.h"
#include "RNUITableViewCell.h"

namespace RN
{
	namespace UI
	{
		class TableView;
		class TableViewDataSource
		{
		public:
			virtual size_t NumberOfRowsInTableView(TableView *tableView) = 0;
			virtual TableViewCell *CellForRowInTableView(TableView *tableView, size_t row) = 0;
			
			virtual float HeightOfRowInTableView(TableView *tableView, size_t row) { return 20.0f; }
		};
		
		class TableView : public ScrollView, public ScrollViewDelegate
		{
		public:
			TableView();
			
			void SetDataSource(TableViewDataSource *dataSource);
			
			TableViewCell *DequeCellWithIdentifier(String *identifier);
			
			void LayoutSubviews() override;
			void SetFrame(const Rect& frame) override;
			
			void ReloadData();
			
		private:
			void Initialize();
			void EnqueueCell(TableViewCell *cell, bool cleanRowData);
			
			size_t RowForContentOffset(float offset);
			float OffsetForRow(size_t row);
			
			void ClearAllCells();
			void InsertCellForRow(size_t row, float offset);
			void UpdateVisibleRows();
			
			void ScrollViewDidScroll(ScrollView *view) override;
			
			TableViewDataSource *_dataSource;
			
			Dictionary _queuedCells;
			Array _cells;
			std::unordered_set<size_t> _visibleCells;
			
			float _height;
			size_t _rows;
			
			RNDefineMeta(TableView, ScrollView)
		};
	}
}

#endif /* __RAYNE_UITABLEVIEW_H__ */
