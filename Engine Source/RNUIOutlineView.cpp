//
//  RNUIOutlineView.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIOutlineView.h"
#include "RNUIOutlineViewCell.h"

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(OutlineView)
		
		OutlineView::OutlineView()
		{
			TableView::SetDataSource(this);
			TableView::SetDelegate(this);
			
			Initialize();
		}
		
		OutlineView::~OutlineView()
		{
			for(ProxyItem *item : _items)
			{
				InvalidateProxyTreeItem(item);
			}
		}
		
		
		void OutlineView::Initialize()
		{
			_dataSource = nullptr;
			_delegate   = nullptr;
		}
		
		
		
		void OutlineView::SetDataSource(OutlineViewDataSource *dataSource)
		{
			_dataSource = dataSource;
		}
		
		void OutlineView::SetDelegate(OutlineViewDelegate *delegate)
		{
			_delegate = delegate;
		}
		
		
		
		void OutlineView::ReloadData()
		{
			RecreateProxyTree();
			TableView::ReloadData();
		}
		
		void OutlineView::ReloadItem(void *item, bool reloadChildren)
		{
		}
		
		
		OutlineViewCell *OutlineView::DequeCellWithIdentifier(String *identifier)
		{
			TableViewCell *cell = TableView::DequeCellWithIdentifier(identifier);
			return static_cast<OutlineViewCell *>(cell);
		}
		
		
		// ---------------------
		// MARK: -
		// MARK: Item expansion
		// ---------------------
		
		void OutlineView::ExpandProxyItem(ProxyItem *item, size_t row, bool recursively)
		{
			if(item->isLeaf)
				return;
			
			if(item && !item->expanded)
			{
				if(_delegate)
					_delegate->OutlineViewWillExpandItem(this, item->item);
				
				item->expanded = true;
				PopulateProxyItem(item);
				
				if(row != k::NotFound)
				{
					std::vector<ProxyItem *> childs;
					GetVisibleItemsForProxyItem(item, childs);
					
					auto iterator = _rows.begin();
					std::advance(iterator, row + 1);
					
					_rows.insert(iterator, childs.begin(), childs.end());
					TableView::InsertRows(row, childs.size());
				}
				
				if(_delegate)
					_delegate->OutlineViewDidExpandItem(this, item->item);
			}
			
			if(recursively)
			{
				for(ProxyItem *proxy : item->children)
				{
					row = GetRowForProxyItem(proxy);
					ExpandProxyItem(proxy, row, true);
				}
			}
		}
		
		void OutlineView::CollapseProxyItem(ProxyItem *item, size_t row, bool recursively)
		{
			if(item->isLeaf)
				return;
			
			if(item && item->expanded)
			{
				if(_delegate)
					_delegate->OutlineViewWillCollapseItem(this, item->item);
				
				item->expanded = false;
				
				if(row != k::NotFound)
				{
					std::vector<ProxyItem *> childs;
					GetVisibleItemsForProxyItem(item, childs);
					
					auto iterator = _rows.begin();
					std::advance(iterator, row + 1);
					
					auto last = iterator;
					std::advance(last, childs.size());
					
					_rows.erase(iterator, last);
					TableView::DeleteRows(row, childs.size());
				}
				
				if(_delegate)
					_delegate->OutlineViewDidCollapseItem(this, item->item);
			}
			
			if(recursively)
			{
				for(ProxyItem *proxy : item->children)
				{
					row = GetRowForProxyItem(proxy);
					CollapseProxyItem(proxy, row, true);
				}
			}
		}
		
		
		
		
		void OutlineView::ExpandItem(void *titem, bool expandChildren)
		{
			size_t row = GetRowForItem(titem);
			ProxyItem *item = (row == k::NotFound) ? FindProxyItemSlowPath(titem) : _rows[row];
			
			if(titem)
			{
				TableView::BegindEditing();
				ExpandProxyItem(item, row, expandChildren);
				TableView::EndEditing();
			}
			else
			{
				TableView::BegindEditing();
				
				for(ProxyItem *item : _items)
				{
					ExpandProxyItem(item, GetRowForProxyItem(item), expandChildren);
				}
				
				TableView::EndEditing();
			}
		}
		
		void OutlineView::CollapseItem(void *titem, bool collapseChildren)
		{
			size_t row = GetRowForItem(titem);
			ProxyItem *item = (row == k::NotFound) ? FindProxyItemSlowPath(titem) : _rows[row];
			
			if(titem)
			{
				TableView::BegindEditing();
				CollapseProxyItem(item, row, collapseChildren);
				TableView::EndEditing();
			}
			else
			{
				TableView::BegindEditing();
				
				for(ProxyItem *item : _items)
				{
					CollapseProxyItem(item, GetRowForProxyItem(item), collapseChildren);
				}
				
				TableView::EndEditing();
			}
		}
		
		
		
		size_t OutlineView::GetRowForProxyItem(ProxyItem *item)
		{
			for(size_t i = 0; i < _rows.size(); i ++)
			{
				if(_rows[i] == item)
					return i;
			}
			
			return k::NotFound;
		}
		
		size_t OutlineView::GetRowForItem(void *item)
		{
			for(size_t i = 0; i < _rows.size(); i ++)
			{
				if(_rows[i]->item == item)
					return i;
			}
			
			return k::NotFound;
		}
		
		void *OutlineView::GetItemForRow(size_t row)
		{
			RN_ASSERT(row < _rows.size(), "Row mustn't exceed the actual row count");
			return _rows.at(row);
		}
		
		
		
		void OutlineView::GetVisibleItemsForProxyItem(ProxyItem *item, std::vector<ProxyItem *>& items)
		{
			for(ProxyItem *child : item->children)
			{
				items.push_back(child);
				
				if(child->expanded)
					GetVisibleItemsForProxyItem(child, items);
			}
		}
		
		OutlineView::ProxyItem *OutlineView::FindProxyItemSlowPath(ProxyItem *item, void *toFind)
		{
			if(item->item == toFind)
				return item;
			
			for(ProxyItem *child : item->children)
			{
				ProxyItem *temp = FindProxyItemSlowPath(child, toFind);
				if(temp)
					return temp;
			}
			
			return nullptr;
		}
		
		OutlineView::ProxyItem *OutlineView::FindProxyItemSlowPath(void *item)
		{
			for(ProxyItem *proxy : _items)
			{
				ProxyItem *temp = FindProxyItemSlowPath(proxy, item);
				if(temp)
					return temp;
			}
			
			return nullptr;
		}
		
		// ---------------------
		// MARK: -
		// MARK: Proxy tree
		// ---------------------
		
		void OutlineView::InvalidateProxyTreeItem(ProxyItem *item)
		{
			for(ProxyItem *child : item->children)
			{
				InvalidateProxyTreeItem(child);
			}
			
			delete item;
		}
		
		void OutlineView::RecreateProxyTree()
		{
			// Clean the old data
			for(ProxyItem *item : _items)
			{
				InvalidateProxyTreeItem(item);
			}
			
			_items.clear();
			_rows.clear();
			
			// Get the new proxy tree
			size_t count = _dataSource->OutlineViewGetNumberOfChildrenForItem(this, nullptr);
			for(size_t i = 0; i < count; i ++)
			{
				void *data = _dataSource->OutlineViewGetChildOfItem(this, nullptr, i);
				
				ProxyItem *item = new ProxyItem(data, 0);
				item->isLeaf = !(_dataSource->OutlineViewItemIsExpandable(this, data));
				
				PopulateProxyItem(item);
				
				_items.push_back(item);
				_rows.push_back(item);
			}
		}
		
		void OutlineView::PopulateProxyItem(ProxyItem *item)
		{
			if(!item->fault)
				return;
			
			item->fault = false;
			
			if(!item->isLeaf)
			{
				size_t children = _dataSource->OutlineViewGetNumberOfChildrenForItem(this, item->item);
				
				for(size_t i = 0; i < children; i ++)
				{
					void *data = _dataSource->OutlineViewGetChildOfItem(this, item->item, i);
					
					ProxyItem *child = new ProxyItem(data, item->indentation + 1);
					child->isLeaf = !(_dataSource->OutlineViewItemIsExpandable(this, data));
					
					item->children.push_back(child);
				}
			}
		}
		
		// ---------------------
		// MARK: -
		// MARK: TableViewDataSource
		// ---------------------
		
		size_t OutlineView::TableViewNumberOfRows(TableView *tableView)
		{
			if(!_dataSource)
				return 0;
			
			return _rows.size();
		}
		
		TableViewCell *OutlineView::TableViewCellForRow(TableView *tableView, size_t row)
		{
			OutlineViewCell *cell = _dataSource->OutlineViewGetCellForItem(this, _rows[row]->item);
			return cell;
		}
		
		// ---------------------
		// MARK: -
		// MARK: TableViewDelegate
		// ---------------------
		
		void OutlineView::TableViewDidSelectRow(TableView *tableView, size_t row)
		{
			if(_delegate)
			{
				ProxyItem *item = _rows[row];
				_delegate->OutlineViewDidSelectItem(this, item->item);
			}
		}
		
		void OutlineView::TableViewWillDeselectRow(TableView *tableView, size_t row)
		{
			if(_delegate)
			{
				ProxyItem *item = _rows[row];
				_delegate->OutlineViewWillDeselectItem(this, item->item);
			}
		}
		
		void OutlineView::TableViewSelectionDidChange(TableView *tableView)
		{
			if(_delegate)
				_delegate->OutlineViewSelectionDidChange(this);
		}
		
		uint32 OutlineView::TableViewIndentationForRow(TableView *tableView, size_t row)
		{
			ProxyItem *item = _rows[row];
			return item->indentation;
		}
		
		void OutlineView::TableViewWillDisplayCellForRow(TableView *tableView, TableViewCell *tcell, size_t row)
		{
			ProxyItem *item = _rows[row];
			OutlineViewCell *cell = static_cast<OutlineViewCell *>(tcell);
			
			cell->_expandable = !item->isLeaf;
			cell->_expanded = item->expanded;
			cell->_outlineView = this;
			cell->_item = item->item;
		}
	}
}
