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
#include "RNIndexSet.h"
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
		};
		
		class TableViewDelegate
		{
		public:
			virtual bool CanSelectRowInTableView(TableView *tableView, size_t row) { return true; }
			virtual void DidSelectRowInTableView(TableView *tableView, size_t row) {}
			virtual void WillDeselectRowInTableView(TableView *tableView, size_t row) {}
			
			virtual uint32 IndentationForRowInTableView(TableView *tableView, size_t row) { return 0; }
			virtual float HeightOfRowInTableView(TableView *tableView, size_t row) { return 20.0f; }
			
			virtual void WillDisplayCellForRowInTableView(TableView *tableView, TableViewCell *cell, size_t row) {}
		};
		
		class TableView : public ScrollView, public ScrollViewDelegate
		{
		public:
			friend class TableViewCell;
			
			TableView();
			~TableView();
			
			void SetDataSource(TableViewDataSource *dataSource);
			void SetDelegate(TableViewDelegate *delegate);
			
			void InsertRows(size_t row, size_t count);
			void DeleteRows(size_t row, size_t count);
			void UpdateRows(size_t row, size_t count);
			
			TableViewCell *DequeCellWithIdentifier(String *identifier);
			
			void LayoutSubviews() override;
			void SetFrame(const Rect& frame) override;
			void SetAllowsMultipleSelection(bool multipleSelection);
			void SetSelection(IndexSet *selection);
			void SetIndentationOffset(float offset);
			void SetRowHeight(float rowHeight);
			
			IndexSet *GetSelection() const { return _selection; }
			bool GetAllowsMultipleSelection() const { return _allowsMultipleSelection; }
			float GetIndentationOffset() const { return _indentationOffset; }
			Range GetVisibleRange() const;
			
			void ReloadData();
			
		private:
			void Initialize();
			void ScrollViewDidScroll(ScrollView *view) override;
			void EnqueueCell(TableViewCell *cell, bool cleanRowData);
			
			size_t GetRowForContentOffset(float offset) const;
			float GetOffsetForRow(size_t row) const;
			float GetIndentationForRow(size_t row) const;
			float GetHeightForRow(size_t row) const;
			
			TableViewCell *GetCellForRow(size_t row);
			
			void ClearAllCells();
			void InsertCellForRow(size_t row, float offset);
			void InvalidateCellsForRange(const Range& range);
			void UpdateDimensions();
			void UpdateVisibleRows(bool updateFrames);
			
			void ConsiderCellForSelection(TableViewCell *cell);
			void DeselectCell(TableViewCell *cell);
			void __AdoptSelection(IndexSet *selection);
			
			TableViewDataSource *_dataSource;
			TableViewDelegate *_delegate;
			
			Dictionary _queuedCells;
			Array _cells;
			std::unordered_set<size_t> _visibleCells;
			
			float _height;
			float _rowHeight;
			float _indentationOffset;
			size_t _rows;
			
			bool _allowsMultipleSelection;
			IndexSet *_selection;
			
			RNDefineMeta(TableView, ScrollView)
		};
	}
}

#endif /* __RAYNE_UITABLEVIEW_H__ */
