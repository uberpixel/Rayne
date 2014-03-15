//
//  RNUIOutlineView.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIOutlineView.h"
#include "RNUIOutlineViewCell.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(OutlineView, TableView)
		
		OutlineView::OutlineView()
		{
			TableView::SetDataSource(this);
			TableView::SetDelegate(this);
			
			Initialize();
		}
		
		OutlineView::~OutlineView()
		{
			for(ProxyItem *item : _items)
				delete item;
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
			if(item)
			{
				ProxyItem *proxy = FindProxyItemSlowPath(item);
				if(proxy)
				{
					TableView::BegindEditing();
					
					size_t row = GetRowForProxyItem(proxy);
					if(row != k::NotFound)
					{
						if(proxy->expanded)
						{
							std::vector<ProxyItem *> items;
							GetVisibleItemsForProxyItem(proxy, items);
							
							auto start = _rows.begin() + row + 1;
							auto end = start + items.size();

							_rows.erase(start, end);
							
							TableView::DeleteRows(row + 1, items.size());
						}
						else
						{
							TableView::UpdateRows(row, 1);
						}
					}
					
					UpdateProxyItem(proxy, reloadChildren);
					
					if(row != k::NotFound && proxy->expanded)
					{
						std::vector<ProxyItem *> items;
						GetVisibleItemsForProxyItem(proxy, items);
						
						auto start = _rows.begin() + row + 1;
						_rows.insert(start, items.begin(), items.end());
						
						TableView::InsertRows(row + 1, items.size());
					}
					
					TableView::EndEditing();
				}
			}
			else
			{
				TableView::BegindEditing();
				
				size_t count = _dataSource->OutlineViewGetNumberOfChildrenForItem(this, nullptr);
				std::unordered_set<ProxyItem *> found;
				
				for(size_t i = 0; i < count; i ++)
				{
					void *item = _dataSource->OutlineViewGetChildOfItem(this, nullptr, i);
					
					for(ProxyItem *proxy : _items)
					{
						if(proxy->item == item)
						{
							found.insert(proxy);
							break;
						}
					}
				}
				
				// Get rid of the deleted proxies
				for(auto i = _items.begin(); i != _items.end();)
				{
					ProxyItem *proxy = *i;
					
					if(found.find(proxy) == found.end())
					{
						size_t row = GetRowForProxyItem(proxy);
						if(row != k::NotFound)
						{
							if(proxy->expanded)
							{
								std::vector<ProxyItem *> items;
								GetVisibleItemsForProxyItem(proxy, items);
								
								auto start = _rows.begin() + row + 1;
								auto end = start + items.size();
								
								_rows.erase(start, end);
								
								TableView::DeleteRows(row + 1, items.size());
							}
							
							TableView::DeleteRows(row, 1);
							_rows.erase(_rows.begin() + row);
						}
						
						delete proxy;
						i = _items.erase(i);
						continue;
					}
					
					i ++;
				}
				
				// Insert new proxies
				for(size_t i = 0; i < count; i ++)
				{
					void *item = _dataSource->OutlineViewGetChildOfItem(this, nullptr, i);
					
					if(i >= _items.size())
					{
						ProxyItem *proxy = new ProxyItem(item, 0);
						proxy->isLeaf = !(_dataSource->OutlineViewItemIsExpandable(this, item));
						
						PopulateProxyItem(proxy);
						
						_items.push_back(proxy);
						_rows.push_back(proxy);
						
						TableView::InsertRows(_rows.size() - 1, 1);
					}
					else if(_items[i]->item != item)
					{
						ProxyItem *proxy = new ProxyItem(item, 0);
						proxy->isLeaf = !(_dataSource->OutlineViewItemIsExpandable(this, item));
						
						PopulateProxyItem(proxy);
						
						size_t row = GetRowForProxyItem(_items[i]);
						
						_items.insert(_items.begin() + i, proxy);
						_rows.insert(_rows.begin() + row, proxy);
						
						TableView::InsertRows(row, 1);
					}
					else if(reloadChildren)
					{
						//ReloadItem(item, true);
					}
				}
				
				TableView::EndEditing();
			}
		}
		
		
		OutlineViewCell *OutlineView::DequeCellWithIdentifier(String *identifier)
		{
			TableViewCell *cell = TableView::DequeCellWithIdentifier(identifier);
			return static_cast<OutlineViewCell *>(cell);
		}
		
		
		// ---------------------
		// MARK: -
		// MARK: Item handling
		// ---------------------
		
		
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
			return _rows.at(row)->item;
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
		
		void OutlineView::RecreateProxyTree()
		{
			// Clean the old data
			for(ProxyItem *item : _items)
				delete item;
			
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
		
		void OutlineView::UpdateProxyItem(ProxyItem *item, bool recursively)
		{
			if(item->fault)
				return;
			
			if(!item->isLeaf)
			{
				size_t children = item->children.size();
				std::vector<void *> newItems;
				std::unordered_set<void *> deletedItems;
				
				// Get all new and deleted items
				for(size_t i = 0; i < children; i ++)
				{
					ProxyItem *proxy = item->children[i];
					deletedItems.insert(proxy->item);
				}
					
				children = _dataSource->OutlineViewGetNumberOfChildrenForItem(this, item->item);
				for(size_t i = 0; i < children; i ++)
				{
					void *data = _dataSource->OutlineViewGetChildOfItem(this, item->item, i);
					deletedItems.erase(data);
					newItems.push_back(data);
				}
				
				// Sweep over the current childs, kick the deleted ones and populate the proxy map
				std::unordered_map<void *, ProxyItem *> proxyMap;
				
				for(auto i = item->children.begin(); i != item->children.end();)
				{
					ProxyItem *proxy = *i;
					
					if(deletedItems.find(proxy->item) != deletedItems.end())
					{
						delete proxy;
						
						i = item->children.erase(i);
						continue;
					}
					
					proxyMap.insert(std::unordered_map<void *, ProxyItem *>::value_type(proxy->item, proxy));
					i ++;
				}
				
				// Recreate the children vector
				children = newItems.size();
				item->children.clear();
				
				for(size_t i = 0; i < children; i ++)
				{
					void *data = newItems[i];
					auto iterator = proxyMap.find(data);
					
					ProxyItem *proxy = (iterator != proxyMap.end()) ? iterator->second : nullptr;
					
					if(!proxy)
					{
						proxy = new ProxyItem(data, item->indentation + 1);
						proxy->isLeaf = !(_dataSource->OutlineViewItemIsExpandable(this, data));
					}
					else
					{
						bool isLeaf = !(_dataSource->OutlineViewItemIsExpandable(this, data));
						if(isLeaf != proxy->isLeaf)
						{
							if(!isLeaf)
							{
								size_t size = proxy->children.size();
								
								for(size_t j = 0; j < size; j ++)
									delete proxy->children[j];
								
								proxy->children.clear();
							}
						}
						
						proxy->isLeaf = isLeaf;
					}
					
					item->children.push_back(proxy);
				}
			}
			
			if(!item->isLeaf && recursively)
			{
				size_t children = item->children.size();
				for(size_t i = 0; i < children; i ++)
				{
					ProxyItem *proxy = item->children[i];
					UpdateProxyItem(proxy, true);
				}
			}
		}
		
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
