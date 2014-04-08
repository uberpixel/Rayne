//
//  RNUITableViewCell.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
			
			RNAPI TableViewCell(String *identifier);
			RNAPI ~TableViewCell() override;
			
			RNAPI virtual void PrepareForReuse();
			
			RNAPI String *GetIdentifier() const { return _identifier; }
			
			RNAPI View *GetContentView() const { return _contentView; }
			RNAPI ImageView *GetImageView() const { return _imageView; }
			RNAPI Label *GetTextLabel() const { return _textLabel; }
			
			RNAPI void SetSelected(bool selected) override;
			RNAPI void SetFrame(const Rect& frame) override;
			RNAPI void LayoutSubviews() override;
			
		protected:
			RNAPI bool PostEvent(EventType event) override;
			
		private:
			void Initialize();
			void SetIndentation(float indentation);
			
			String *_identifier;
			TableView *_tableView;
			
			View *_contentView;
			ImageView *_imageView;
			Label *_textLabel;
			
			float _offset;
			float _indentation;
			size_t _row;
			
			RNDeclareMeta(TableViewCell)
		};
	}
}

#endif /* __RAYNE_UITABLEVIEWCELL_H__ */
