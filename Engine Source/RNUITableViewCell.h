//
//  RNUITableViewCell.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UITABLEVIEWCELL_H__
#define __RAYNE_UITABLEVIEWCELL_H__

#include "RNBase.h"
#include "RNString.h"
#include "RNUIControl.h"
#include "RNUILabel.h"
#include "RNUIImageView.h"

namespace RN
{
	namespace UI
	{
		class TableView;
		class TableViewCell : public Control
		{
		public:
			friend class TableView;
			
			TableViewCell(String *identifier);
			~TableViewCell() override;
			
			virtual void PrepareForReuse();
			
			String *GetIdentifier() const { return _identifier; }
			
			ImageView *GetImageView() const { return _imageView; }
			Label *GetTextLabel() const { return _textLabel; }
			
			void SetSelected(bool selected) override;
			void SetFrame(const Rect& frame) override;
			void LayoutSubviews() override;
			
		protected:
			bool PostEvent(EventType event) override;
			
		private:
			void Initialize();
			
			String *_identifier;
			TableView *_tableView;
			
			View *_contentView;
			ImageView *_imageView;
			Label *_textLabel;
			
			float _offset;
			size_t _row;
			
			RNDefineMeta(TableViewCell, View)
		};
	}
}

#endif /* __RAYNE_UITABLEVIEWCELL_H__ */
