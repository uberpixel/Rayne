//
//  RNUITableView.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUITableView.h"
#include "RNInput.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(TableView, ScrollView)
		
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
			_rowHeight = 20.0f;
			_height = 0.0f;
			_rows = _changeRows = 0;
			
			_allowsMultipleSelection = false;
			_editing = false;
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
			RN_ASSERT(_editing == false, "ReloadData() is forbidden between BeginEditing() EndEditing() calls");
			
			_rows = _dataSource->TableViewNumberOfRows(this);
			_changeRows = _rows;
			
			UpdateDimensions();
			ClearAllCells();
			UpdateVisibleRows(false);
		}
		
		void TableView::UpdateDimensions()
		{
			_height = _rows * _rowHeight;
			SetContentSize(Vector2(GetFrame().width, _height));
		}
		
		void TableView::SetFrame(const Rect& frame)
		{
			ScrollView::SetFrame(frame);
			SetContentSize(Vector2(frame.width, _height));
			UpdateVisibleRows(false);
		}
		
		void TableView::SetIndentationOffset(float offset)
		{
			_indentationOffset = offset;
			UpdateVisibleRows(false);
		}
		
		void TableView::ScrollViewDidScroll(ScrollView *view)
		{
			UpdateVisibleRows(false);
		}
		
		void TableView::SetRowHeight(float rowHeight)
		{
			_rowHeight = rowHeight;
			ReloadData();
		}
		
		
		// ---------------------
		// MARK: -
		// MARK: Row handling
		// ---------------------
		
		size_t TableView::GetRowForContentOffset(float offset) const
		{
			size_t row = static_cast<size_t>(floorf(offset / _rowHeight));
			return std::min(_rows, row);
		}
		
		float TableView::GetOffsetForRow(size_t row) const
		{
			RN_ASSERT(row <= _rows, "Invalid row number");
			return row * _rowHeight;
		}
		
		float TableView::GetIndentationForRow(size_t row) const
		{
			if(!_delegate)
				return 0.0f;
			
			return _delegate->TableViewIndentationForRow(const_cast<TableView *>(this), row) * _indentationOffset;
		}
		
		float TableView::GetHeightForRow(size_t row) const
		{
			if(!_delegate)
				return _rowHeight;
			
			return _rowHeight;
		}
		
		Range TableView::GetVisibleRange() const
		{
			Range range;
			
			float offset = GetContentOffset().y;
			float contentHeight = offset + GetFrame().height;
			
			size_t firstVisibleRow = GetRowForContentOffset(offset);
			size_t lastVisibleRow  = GetRowForContentOffset(contentHeight);
			
			lastVisibleRow = std::min(lastVisibleRow + 1, _rows);
			
			range.origin = firstVisibleRow;
			range.length = lastVisibleRow - firstVisibleRow;
			
			return range;
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
		
		
		void TableView::BegindEditing()
		{
			RN_ASSERT(_dataSource, "TableView needs a data source!");
			RN_ASSERT(_editing == false, "BeginEditing() can't be stacked!\n");
			
			_editing = true;
		}
		
		void TableView::EndEditing()
		{
			RN_ASSERT(_dataSource, "TableView needs a data source!");
			RN_ASSERT(_editing, "EndEditing() mustn't be called without a prior BeginEditing() call");
			
			_editing = false;
			CarryOutChanges();
		}
		
		
		
		void TableView::InsertRows(size_t row, size_t count)
		{
			RN_ASSERT(_dataSource, "TableView needs a data source!");
			RN_ASSERT(_changeRows >= row, "Row range out of bounds");
			
			_changes.emplace_back(EditingSet(EditingSet::Type::Insertion, row, count));
			_changeRows += count;
			
			if(!_editing)
				CarryOutChanges();
		}
		
		void TableView::DeleteRows(size_t row, size_t count)
		{
			RN_ASSERT(_dataSource, "TableView needs a data source!");
			RN_ASSERT(_changeRows >= row + count, "Row range out of bounds");
			
			_changes.emplace_back(EditingSet(EditingSet::Type::Deletion, row, count));
			_changeRows -= count;
			
			if(!_editing)
				CarryOutChanges();
		}
		
		void TableView::UpdateRows(size_t row, size_t count)
		{
			RN_ASSERT(_changeRows >= row + count, "Row range out of bounds");
			
			if(!_editing)
			{
				InvalidateCellsForRange(Range(row, count));
				UpdateVisibleRows(true);
			}
			else
			{
				_changes.emplace_back(EditingSet(EditingSet::Type::Update, row, count));
			}
		}
		
		void TableView::CarryOutChanges()
		{
			size_t insertedRows = 0;
			size_t deletedRows  = 0;
			
			std::vector<size_t> indices = _selection->GetIndices();
			
			for(size_t i = 0; i < _changes.size(); i ++)
			{
				EditingSet& set = _changes[i];
				ptrdiff_t offset = 0;
				
				switch(set.type)
				{
					case EditingSet::Type::Insertion:
						offset = set.count;
						insertedRows += set.count;
						
						for(auto iterator = indices.begin(); iterator != indices.end(); iterator ++)
						{
							if(*iterator >= set.row)
								*iterator += set.count;
						}
						break;
						
					case EditingSet::Type::Deletion:
						offset = set.count;
						offset = -offset;

						deletedRows += set.count;
						
						for(auto iterator = indices.begin(); iterator != indices.end();)
						{
							if(*iterator >= set.row && set.row + set.count >= *iterator)
							{
								iterator = indices.erase(iterator);
								continue;
							}
							else
							if(*iterator >= set.row + set.count)
								*iterator += offset;
							
							iterator ++;
						}
						
						break;
						
					default:
						break;
				}
				
				// Apply the new offset if needed
				if(offset != 0)
				{
					for(size_t j = i + 1; j < _changes.size(); j ++)
					{
						EditingSet& set = _changes[j];
						set.row += offset;
					}
				}
			}
			
			// Sanity check
			_rows += insertedRows;
			_rows -= deletedRows;
			
			size_t rows = _dataSource->TableViewNumberOfRows(this);
			RN_ASSERT(_rows == rows, "Invalid row count after EndEditing() call");
			
			// Insertion update
			IndexSet *set = new IndexSet();
			for(size_t index : indices)
			{
				set->AddIndex(index);
			}
			
			_selection->Release();
			_selection = set;
			
			_changeRows = rows;
			
			// Update
			_changes.clear();
			
			InvalidateCellsForRange(GetVisibleRange());
			UpdateDimensions();
			UpdateVisibleRows(true);
		}
		
		
		void TableView::ScrollToRow(size_t row, ScrollPosition position)
		{
			Vector2 offset = GetContentOffset();
			float rowOffset = row * _rowHeight;
			float height = GetFrame().height;
			
			if(position == ScrollPosition::None && offset.y >= rowOffset && offset.y <= (rowOffset + height) - _rowHeight)
				return;
			
			switch(position)
			{
				case ScrollPosition::None:
				case ScrollPosition::Top:
					offset.y = rowOffset;
					break;
					
				case ScrollPosition::Center:
					offset.y = roundf(rowOffset + ((height * 0.5) - (_rowHeight * 0.5)));
					break;
					
				case ScrollPosition::Bottom:
					offset.y = rowOffset + (height - _rowHeight);
					break;
			}
			
			const Vector2& size = GetContentSize();
			
			offset.y = std::max(0.0f, std::min(size.y - height, offset.y));
			
			SetContentOffset(offset);
			UpdateVisibleRows(false);
		}
		
		
		void TableView::InvalidateCellsForRange(const Range& range)
		{
			for(size_t i = 0; i < _cells.GetCount(); i ++)
			{
				TableViewCell *cell = _cells.GetObjectAtIndex<TableViewCell>(i);
				
				if(cell->_row >= range.origin && cell->_row <= range.GetEnd())
				{
					EnqueueCell(cell, true);
					i --;
					
					continue;
				}
			}
		}
		
		void TableView::InvalidateCellsForIndexSet(IndexSet *set)
		{
			for(size_t i = 0; i < _cells.GetCount(); i ++)
			{
				TableViewCell *cell = _cells.GetObjectAtIndex<TableViewCell>(i);
				
				if(set->ContainsIndex(cell->_row))
				{
					EnqueueCell(cell, true);
					i --;
					
					continue;
				}
			}
		}
		
		
		void TableView::UpdateVisibleRows(bool updateFrames)
		{
			if(!_dataSource)
				return;
			
			// Find the first and last visible row
			Range range = std::move(GetVisibleRange());
			
			// Remove invisible rows
			for(size_t i = 0; i < _cells.GetCount(); i ++)
			{
				TableViewCell *cell = _cells.GetObjectAtIndex<TableViewCell>(i);
				if(cell->_row < range.origin || cell->_row > range.GetEnd())
				{
					EnqueueCell(cell, true);
					i --;
					
					continue;
				}
				
				if(updateFrames)
				{
					float offset = GetOffsetForRow(cell->_row);
					float height = GetHeightForRow(cell->_row);
					float width  = GetFrame().width;
					
					cell->SetFrame(Rect(0.0f, offset, width, height));
					cell->SetIndentation(GetIndentationForRow(cell->_row));
				}
			}
			
			// Indentation
			if(!updateFrames)
			{
				for(size_t i = 0; i < _cells.GetCount(); i ++)
				{
					TableViewCell *cell = _cells.GetObjectAtIndex<TableViewCell>(i);
					cell->SetIndentation(GetIndentationForRow(cell->_row));
				}
			}
			
			// Insert missing rows
			for(size_t i = range.origin; i < range.origin + range.length; i ++)
			{
				if(_visibleCells.find(i) == _visibleCells.end())
				{
					InsertCellForRow(i, GetOffsetForRow(i));
				}
			}
		}
		
		void TableView::LayoutSubviews()
		{
			ScrollView::LayoutSubviews();
			
			float width = GetFrame().width;
			
			size_t count = _cells.GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				TableViewCell *cell = _cells.GetObjectAtIndex<TableViewCell>(i);
				Rect frame = cell->GetFrame();
				frame.width = width;
				
				cell->SetFrame(frame);
			}
		}
		
		void TableView::InsertCellForRow(size_t row, float offset)
		{
			TableViewCell *cell = _dataSource->TableViewCellForRow(this, row);
			RN_ASSERT(cell, "TableViewCellForRow() must not return NULL!");
			
			float height = GetHeightForRow(row);
			float indentation = GetIndentationForRow(row);
			float width = GetFrame().width;
			
			cell->SetFrame(Rect(0.0f, offset, width, height));
			cell->SetIndentation(indentation);
			cell->SetNeedsLayoutUpdate();
			cell->_tableView = this;
			cell->_offset = offset;
			cell->_row = row;
			
			if(_selection->ContainsIndex(row))
				cell->SetSelected(true);
			
			_cells.AddObject(cell);
			_visibleCells.insert(row);
			
			if(_delegate)
				_delegate->TableViewWillDisplayCellForRow(this, cell, row);
			
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
					_delegate->TableViewWillDeselectRow(this, row);
				
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
					_delegate->TableViewDidSelectRow(this, row);
				
				TableViewCell *cell = GetCellForRow(row);
				if(cell)
					cell->SetSelected(true);
			}
			
			if(_delegate)
				_delegate->TableViewSelectionDidChange(this);
		}
		
		void TableView::ConsiderCellForSelection(TableViewCell *cell)
		{
			bool canAdd = (!_delegate || _delegate->TableViewCanSelectRow(this, cell->_row));
			if(canAdd)
			{
				Input *input = Input::GetSharedInstance();
				
#if RN_PLATFORM_MAC_OS
				bool ctrlDown = (input->GetModifierKeys() & KeyCommand);
#else
				bool ctrlDown = (input->GetModifierKeys() & KeyControl);
#endif
				
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
							_delegate->TableViewDidSelectRow(this, cell->_row);
						
						cell->SetSelected(true);
						
						if(_delegate)
							_delegate->TableViewSelectionDidChange(this);
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
				_delegate->TableViewWillDeselectRow(this, cell->_row);
			
			_selection->RemoveIndex(cell->_row);
			cell->SetSelected(false);
			
			if(_delegate)
				_delegate->TableViewSelectionDidChange(this);
		}
	}
}
