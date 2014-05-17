//
//  RNUITableView.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
			RNAPI virtual size_t TableViewNumberOfRows(TableView *tableView) = 0;
			RNAPI virtual TableViewCell *TableViewCellForRow(TableView *tableView, size_t row) = 0;
		};
		
		class TableViewDelegate
		{
		public:
			RNAPI virtual bool TableViewCanSelectRow(TableView *tableView, size_t row) { return true; }
			RNAPI virtual void TableViewDidSelectRow(TableView *tableView, size_t row) {}
			RNAPI virtual void TableViewWillDeselectRow(TableView *tableView, size_t row) {}
			RNAPI virtual void TableViewSelectionDidChange(TableView *tableView) {}
			
			RNAPI virtual uint32 TableViewIndentationForRow(TableView *tableView, size_t row) { return 0; }
			RNAPI virtual void TableViewWillDisplayCellForRow(TableView *tableView, TableViewCell *cell, size_t row) {}
		};
		
		class TableView : public ScrollView, public ScrollViewDelegate
		{
		public:
			friend class TableViewCell;
			
			enum class ScrollPosition
			{
				None,
				Top,
				Center,
				Bottom
			};
			
			RNAPI TableView();
			RNAPI ~TableView();
			
			RNAPI void SetDataSource(TableViewDataSource *dataSource);
			RNAPI void SetDelegate(TableViewDelegate *delegate);
			
			RNAPI void BegindEditing();
			RNAPI void EndEditing();
			RNAPI void InsertRows(size_t row, size_t count);
			RNAPI void DeleteRows(size_t row, size_t count);
			RNAPI void UpdateRows(size_t row, size_t count);
			
			RNAPI void ScrollToRow(size_t row, ScrollPosition position);
			
			RNAPI TableViewCell *DequeCellWithIdentifier(String *identifier);
			RNAPI TableViewCell *GetCellForRow(size_t row);
			
			RNAPI void LayoutSubviews() override;
			RNAPI void SetFrame(const Rect& frame) override;
			RNAPI void SetAllowsMultipleSelection(bool multipleSelection);
			RNAPI void SetSelection(IndexSet *selection);
			RNAPI void SetIndentationOffset(float offset);
			RNAPI void SetRowHeight(float rowHeight);
			
			RNAPI IndexSet *GetSelection() const { return _selection; }
			RNAPI bool GetAllowsMultipleSelection() const { return _allowsMultipleSelection; }
			RNAPI float GetIndentationOffset() const { return _indentationOffset; }
			RNAPI Range GetVisibleRange() const;
			
			RNAPI void ReloadData();
			
		private:
			struct EditingSet
			{
				enum class Type
				{
					Insertion,
					Deletion,
					Update
				};
				
				EditingSet(Type ttype, size_t trow, size_t tcount)
				{
					type  = ttype;
					row   = trow;
					count = tcount;
				}
				
				Type type;
				size_t row;
				size_t count;
			};
			
			void Initialize();
			RNAPI void ScrollViewDidScroll(ScrollView *view) override;
			RNAPI void ScrollViewDidChangeScrollerInset(ScrollView *scrollView, const EdgeInsets &insets) override;
			void EnqueueCell(TableViewCell *cell, bool cleanRowData);
			
			size_t GetRowForContentOffset(float offset) const;
			float GetOffsetForRow(size_t row) const;
			float GetIndentationForRow(size_t row) const;
			float GetHeightForRow(size_t row) const;
			
			void ClearAllCells();
			void InsertCellForRow(size_t row, float offset);
			void InvalidateCellsForRange(const Range& range);
			void InvalidateCellsForIndexSet(IndexSet *set);
			void UpdateDimensions();
			void UpdateVisibleRows(bool updateFrames);
			void CarryOutChanges();
			
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
			bool _editing;
			IndexSet *_selection;
			
			std::vector<EditingSet> _changes;
			size_t _changeRows;
			
			RNDeclareMeta(TableView)
		};
	}
}

#endif /* __RAYNE_UITABLEVIEW_H__ */
