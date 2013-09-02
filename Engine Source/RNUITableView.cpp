//
//  RNUITableView.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUITableView.h"
#include "RNInput.h"

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(TableView)
		
		TableView::TableView()
		{
			Initialize();
		}
		
		TableView::~TableView()
		{
			_selection->Release();
		}
		
		void TableView::Initialize()
		{
			ScrollView::SetDelegate(this);
			
			_dataSource = nullptr;
			_delegate = nullptr;
			
			_indentationOffset = 15.0f;
			_height = 0.0f;
			_rows = 0;
			
			_allowsMultipleSelection = false;
			_selection = new IndexSet();
		}
		
		
		void TableView::SetDataSource(TableViewDataSource *dataSource)
		{
			_dataSource = dataSource;
			
			ClearAllCells();
			
			if(_dataSource)
				ReloadData();
		}
		
		void TableView::SetDelegate(TableViewDelegate *delegate)
		{
			_delegate = delegate;
		}
		
		
		
		void TableView::ClearAllCells()
		{
			size_t count = _cells.GetCount();
			
			for(size_t i = 0; i < count; i ++)
			{
				TableViewCell *cell = _cells.GetObjectAtIndex<TableViewCell>(i);
				EnqueueCell(cell, false);
				
				cell->RemoveFromSuperview();
			}
			
			_visibleCells.clear();
			_cells.RemoveAllObjects();
		}
		
		
		
		void TableView::ReloadData()
		{
			RN_ASSERT(_dataSource, "TableView needs a data source!");
			
			ClearAllCells();
			
			_height = 0.0f;
			_rows   = _dataSource->NumberOfRowsInTableView(this);
			
			for(size_t i = 0; i < _rows; i ++)
				_height += _dataSource->HeightOfRowInTableView(this, i);
			
			SetContentSize(Vector2(Frame().width, _height));
			UpdateVisibleRows();
		}
		
		void TableView::SetFrame(const Rect& frame)
		{
			ScrollView::SetFrame(frame);
			SetContentSize(Vector2(frame.width, _height));
			UpdateVisibleRows();
		}
		
		void TableView::ScrollViewDidScroll(ScrollView *view)
		{
			UpdateVisibleRows();
		}
		
		
		// ---------------------
		// MARK: -
		// MARK: Row handling
		// ---------------------
		
		size_t TableView::RowForContentOffset(float offset)
		{
			float height = 0.0f;
			
			for(size_t i = 0; i < _rows; i ++)
			{
				height += _dataSource->HeightOfRowInTableView(this, i);
				
				if(height >= offset)
					return i;
			}
			
			return k::NotFound;
		}
		
		float TableView::OffsetForRow(size_t row)
		{
			RN_ASSERT(row <= _rows, "Invalid row number");
			
			float offset = 0.0f;
			
			for(size_t i = 0; i < row; i ++)
			{
				offset += _dataSource->HeightOfRowInTableView(this, i);
			}
			
			return offset;
		}
		
		TableViewCell *TableView::GetCellForRow(size_t row)
		{
			for(size_t i = 0; i < _cells.GetCount(); i ++)
			{
				TableViewCell *cell = _cells.GetObjectAtIndex<TableViewCell>(i);
				if(cell->_row == row)
					return cell;
			}
			
			return nullptr;
		}
		
		
		void TableView::UpdateVisibleRows()
		{
			if(!_dataSource)
				return;
			
			// Find the first and last visible row
			float offset = GetContentOffset().y;
			float contentHeight = offset + Frame().height;
			
			size_t firstVisibleRow = 0;
			size_t lastVisibleRow  = 0;
			
			float temp = 0.0f;
			
			for(size_t i = 0; (i < _rows && temp <= contentHeight); i ++)
			{
				if(temp <= offset)
					firstVisibleRow = i;
				
				if(temp <= contentHeight)
					lastVisibleRow = std::min(i + 1, _rows);
					
				temp += _dataSource->HeightOfRowInTableView(this, i);
			}
			
			// Remove invisible rows
			for(size_t i = 0; i < _cells.GetCount(); i ++)
			{
				TableViewCell *cell = _cells.GetObjectAtIndex<TableViewCell>(i);
				if(cell->_row < firstVisibleRow || cell->_row > lastVisibleRow)
				{
					EnqueueCell(cell, true);
					i --;
				}
			}
			
			// Insert missing rows
			for(size_t i = firstVisibleRow; i < lastVisibleRow; i ++)
			{
				if(_visibleCells.find(i) == _visibleCells.end())
				{
					InsertCellForRow(i, OffsetForRow(i));
				}
			}
		}
		
		void TableView::LayoutSubviews()
		{
			ScrollView::LayoutSubviews();
			
			float width = Frame().width;
			
			size_t count = _cells.GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				TableViewCell *cell = _cells.GetObjectAtIndex<TableViewCell>(i);
				Rect frame = cell->Frame();
				frame.width = width;
				
				cell->SetFrame(frame);
			}
		}
		
		void TableView::InsertCellForRow(size_t row, float offset)
		{
			TableViewCell *cell = _dataSource->CellForRowInTableView(this, row);
			
			float width = Frame().width;
			float height = _dataSource->HeightOfRowInTableView(this, row);
			
			cell->SetFrame(Rect(0.0f, offset, width, height));
			cell->_tableView = this;
			cell->_offset = offset;
			cell->_row = row;
			
			if(_selection->ContainsIndex(row))
				cell->SetSelected(true);
			
			_cells.AddObject(cell);
			_visibleCells.insert(row);
			
			AddSubview(cell);
		}
		
		
		TableViewCell *TableView::DequeCellWithIdentifier(String *identifier)
		{
			Array *cells = _queuedCells.GetObjectForKey<Array>(identifier);
			if(cells && cells->GetCount() > 0)
			{
				TableViewCell *cell = cells->GetLastObject<TableViewCell>()->Retain();
				cells->RemoveObjectAtIndex(cells->GetCount() - 1);
				
				cell->PrepareForReuse();
				return cell->Autorelease();
			}
			
			return nullptr;
		}
		
		void TableView::EnqueueCell(TableViewCell *cell, bool cleanRowData)
		{
			Array *cells = _queuedCells.GetObjectForKey<Array>(cell->GetIdentifier());
			if(!cells)
			{
				cells = new Array();
				
				_queuedCells.SetObjectForKey(cells->Autorelease(), cell->GetIdentifier());
			}
			
			cells->AddObject(cell);
			cell->RemoveFromSuperview();
			
			if(cleanRowData)
			{
				_cells.RemoveObject(cell);
				_visibleCells.erase(cell->_row);
			}
		}
		
		
		// ---------------------
		// MARK: -
		// MARK: Selection
		// ---------------------
		
		void TableView::SetSelection(IndexSet *selection)
		{
			RN_ASSERT(selection, "Selection mustn't be NULL!");
			__AdoptSelection(selection->Copy());
		}
		
		void TableView::SetAllowsMultipleSelection(bool multipleSelection)
		{
			_allowsMultipleSelection = multipleSelection;
			if(!_allowsMultipleSelection && _selection->GetCount() > 0)
			{
				size_t selection = _selection->GetFirstIndex();
				__AdoptSelection(new IndexSet(selection));
			}
		}
		
		
		void TableView::__AdoptSelection(IndexSet *selection)
		{
			size_t count = _selection->GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				size_t row = _selection->GetIndex(i);
				
				if(_delegate)
					_delegate->WillDeselectRowInTableView(this, row);
				
				TableViewCell *cell = GetCellForRow(row);
				if(cell)
					cell->SetSelected(false);
			}
			
			_selection->Release();
			_selection = selection;
			
			count = _selection->GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				size_t row = _selection->GetIndex(i);
				
				if(_delegate)
					_delegate->DidSelectRowInTableView(this, row);
				
				TableViewCell *cell = GetCellForRow(row);
				if(cell)
					cell->SetSelected(true);
			}
		}
		
		void TableView::ConsiderCellForSelection(TableViewCell *cell)
		{
			bool canAdd = (!_delegate || _delegate->CanSelectRowInTableView(this, cell->_row));
			if(canAdd)
			{
				Input *input = Input::GetSharedInstance();
				bool ctrlDown = (input->GetModifierKeys() & KeyControl);
				
				if(!_allowsMultipleSelection || !ctrlDown)
				{
					__AdoptSelection(new IndexSet(cell->_row));
				}
				else
				{
					if(!_selection->ContainsIndex(cell->_row))
					{
						_selection->AddIndex(cell->_row);
						
						if(_delegate)
							_delegate->DidSelectRowInTableView(this, cell->_row);
						
						cell->SetSelected(true);
					}
					else
					{
						DeselectCell(cell);
					}
				}
			}
		}
		
		void TableView::DeselectCell(TableViewCell *cell)
		{
			if(_delegate)
				_delegate->WillDeselectRowInTableView(this, cell->_row);
			
			_selection->RemoveIndex(cell->_row);
			cell->SetSelected(false);
		}
	}
}
