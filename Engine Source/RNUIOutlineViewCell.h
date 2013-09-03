//
//  RNUIOutlineViewCell.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIOUTLINEVIEWCELL_H__
#define __RAYNE_UIOUTLINEVIEWCELL_H__

#include "RNBase.h"
#include "RNUITableViewCell.h"
#include "RNUIButton.h"

namespace RN
{
	namespace UI
	{
		class OutlineView;
		class OutlineViewCell : public TableViewCell
		{
		public:
			friend class OutlineView;
			
			OutlineViewCell(String *identifier);
			~OutlineViewCell() override;
			
			void PrepareForReuse() override;
			void LayoutSubviews() override;
			
		private:
			void Initialize();
			void DisclosureTriangleClicked();
			
			OutlineView *_outlineView;
			
			
			bool _expandable;
			bool _expanded;
			
			void *_item;
			Button *_disclosureTriangle;
			
			RNDefineMeta(OutlineViewCell, TableViewCell)
		};
	}
}

#endif /* __RAYNE_UIOUTLINEVIEWCELL_H__ */
