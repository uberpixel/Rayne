//
//  RNUIOutlineView.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
			RNAPI virtual bool OutlineViewItemIsExpandable(OutlineView *outlineView, void *item) = 0;
			RNAPI virtual size_t OutlineViewGetNumberOfChildrenForItem(OutlineView *outlineView, void *item) = 0;
			RNAPI virtual void *OutlineViewGetChildOfItem(OutlineView *outlineView, void *item, size_t child) = 0;
			RNAPI virtual OutlineViewCell *OutlineViewGetCellForItem(OutlineView *outlineView, void *item) = 0;
		};
		
		class OutlineViewDelegate
		{
		public:
			RNAPI virtual void OutlineViewWillExpandItem(OutlineView *outlineView, void *item) {}
			RNAPI virtual void OutlineViewDidExpandItem(OutlineView *outlineView, void *item) {}
			RNAPI virtual void OutlineViewWillCollapseItem(OutlineView *outlineView, void *item) {}
			RNAPI virtual void OutlineViewDidCollapseItem(OutlineView *outlineView, void *item) {}
			
			RNAPI virtual void OutlineViewSelectionDidChange(OutlineView *outlineView) {}
			
			RNAPI virtual void OutlineViewWillDeselectItem(OutlineView *outlineView, void *item) {}
			RNAPI virtual void OutlineViewDidSelectItem(OutlineView *outlineView, void *item) {}
		};
		
		
		class OutlineView : public TableView, protected TableViewDelegate, protected TableViewDataSource
		{
		public:
			RNAPI OutlineView();
			RNAPI ~OutlineView();
			
			RNAPI void KeyDown(Event *event) override;
			
			RNAPI void SetDataSource(OutlineViewDataSource *dataSource);
			RNAPI void SetDelegate(OutlineViewDelegate *delegate);
			
			RNAPI void ReloadData();
			RNAPI void ReloadItem(void *item, bool reloadChildren);
			
			RNAPI void ExpandItem(void *item, bool expandChildren);
			RNAPI void CollapseItem(void *item, bool collapseChildren);
			
			RNAPI OutlineViewCell *DequeCellWithIdentifier(String *identifier);
			
			RNAPI size_t GetRowForItem(void *item);
			RNAPI void *GetItemForRow(size_t row);
			
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
				
				~ProxyItem()
				{
					for(size_t i = 0; i < children.size(); i ++)
						delete children[i];
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
			void UpdateProxyItem(ProxyItem *item, bool recursively);
			
			void ExpandProxyItem(ProxyItem *item, size_t row, bool recursively);
			void CollapseProxyItem(ProxyItem *item, size_t row, bool recursively);
			
			ProxyItem *FindProxyItemSlowPath(ProxyItem *item, void *toFind);
			ProxyItem *FindProxyItemSlowPath(void *item);
			void GetVisibleItemsForProxyItem(ProxyItem *item, std::vector<ProxyItem *>& items);
			size_t GetRowForProxyItem(ProxyItem *item);
			
			
			RNAPI size_t TableViewNumberOfRows(TableView *tableView) override;
			RNAPI TableViewCell *TableViewCellForRow(TableView *tableView, size_t row) override;

			RNAPI void TableViewDidSelectRow(TableView *tableView, size_t row) override;
			RNAPI void TableViewWillDeselectRow(TableView *tableView, size_t row) override;
			RNAPI void TableViewSelectionDidChange(TableView *tableView) override;
			
			RNAPI uint32 TableViewIndentationForRow(TableView *tableView, size_t row) override;
			RNAPI void TableViewWillDisplayCellForRow(TableView *tableView, TableViewCell *cell, size_t row) override;
			
			
			OutlineViewDataSource *_dataSource;
			OutlineViewDelegate *_delegate;
			
			std::vector<ProxyItem *> _items;
			std::vector<ProxyItem *> _rows;
			
			RNDeclareMeta(OutlineView)
		};
	}
}

#endif /* __RAYNE_OUTLINEVIEW_H__ */
