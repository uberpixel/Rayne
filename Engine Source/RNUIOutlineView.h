//
//  RNUIOutlineView.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIOUTLINEVIEW_H__
#define __RAYNE_UIOUTLINEVIEW_H__

#include "RNBase.h"
#include "RNUITableView.h"
#include "RNUIOutlineViewCell.h"

namespace RN
{
	namespace UI
	{
		class OutlineView;
		
		class OutlineViewDataSource
		{
		public:
			virtual bool OutlineViewItemIsExpandable(OutlineView *outlineView, void *item) = 0;
			virtual size_t OutlineViewGetNumberOfChildrenForItem(OutlineView *outlineView, void *item) = 0;
			virtual void *OutlineViewGetChildOfItem(OutlineView *outlineView, void *item, size_t child) = 0;
			virtual OutlineViewCell *OutlineViewGetCellForItem(OutlineView *outlineView, void *item) = 0;
		};
		
		class OutlineViewDelegate
		{
		public:
			virtual void OutlineViewWillExpandItem(OutlineView *outlineView, void *item) {}
			virtual void OutlineViewDidExpandItem(OutlineView *outlineView, void *item) {}
			virtual void OutlineViewWillCollapseItem(OutlineView *outlineView, void *item) {}
			virtual void OutlineViewDidCollapseItem(OutlineView *outlineView, void *item) {}
			
			virtual void OutlineViewSelectionDidChange(OutlineView *outlineView) {}
			
			virtual void OutlineViewWillDeselectItem(OutlineView *outlineView, void *item) {}
			virtual void OutlineViewDidSelectItem(OutlineView *outlineView, void *item) {}
		};
		
		
		class OutlineView : public TableView, protected TableViewDelegate, protected TableViewDataSource
		{
		public:
			OutlineView();
			~OutlineView();
			
			void SetDataSource(OutlineViewDataSource *dataSource);
			void SetDelegate(OutlineViewDelegate *delegate);
			
			void ReloadData();
			void ReloadItem(void *item, bool reloadChildren);
			
			void ExpandItem(void *item, bool expandChildren);
			void CollapseItem(void *item, bool collapseChildren);
			
			OutlineViewCell *DequeCellWithIdentifier(String *identifier);
			
			size_t GetRowForItem(void *item);
			void *GetItemForRow(size_t row);
			
		private:
			struct ProxyItem
			{
				ProxyItem(void *titem, uint32 indent)
				{
					item = titem;
					expanded = false;
					isLeaf = false;
					fault = true;
					indentation = indent;
				}
				
				void *item;
				bool expanded;
				bool isLeaf;
				bool fault;
				uint32 indentation;
				
				std::vector<ProxyItem *> children;
			};
			
			void Initialize();
			void RecreateProxyTree();
			void PopulateProxyItem(ProxyItem *item);
			void InvalidateProxyTreeItem(ProxyItem *item);
			
			void ExpandProxyItem(ProxyItem *item, size_t row, bool recursively);
			void CollapseProxyItem(ProxyItem *item, size_t row, bool recursively);
			
			ProxyItem *FindProxyItemSlowPath(ProxyItem *item, void *toFind);
			ProxyItem *FindProxyItemSlowPath(void *item);
			void GetVisibleItemsForProxyItem(ProxyItem *item, std::vector<ProxyItem *>& items);
			size_t GetRowForProxyItem(ProxyItem *item);
			
			
			size_t TableViewNumberOfRows(TableView *tableView) override;
			TableViewCell *TableViewCellForRow(TableView *tableView, size_t row) override;

			void TableViewDidSelectRow(TableView *tableView, size_t row) override;
			void TableViewWillDeselectRow(TableView *tableView, size_t row) override;
			void TableViewSelectionDidChange(TableView *tableView) override;
			
			uint32 TableViewIndentationForRow(TableView *tableView, size_t row) override;
			void TableViewWillDisplayCellForRow(TableView *tableView, TableViewCell *cell, size_t row) override;
			
			
			OutlineViewDataSource *_dataSource;
			OutlineViewDelegate *_delegate;
			
			std::vector<ProxyItem *> _items;
			std::vector<ProxyItem *> _rows;
			
			RNDefineMeta(OutlineView, TableView)
		};
	}
}

#endif /* __RAYNE_OUTLINEVIEW_H__ */
