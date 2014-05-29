//
//  RNUISegmentView.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UISEGMENTVIEW_H__
#define __RAYNE_UISEGMENTVIEW_H__

#include "RNBase.h"
#include "RNArray.h"
#include "RNUIControl.h"
#include "RNUIButton.h"

namespace RN
{
	namespace UI
	{
		class SegmentView : public Control
		{
		public:
			RNAPI SegmentView();
			RNAPI ~SegmentView() override;
			
			RNAPI void InsertSegmentAtIndex(Image *image, size_t index);
			RNAPI void InsertSegmentAtIndex(String *title, size_t index);
			
			RNAPI void RemoveSegmentAtIndex(size_t index);
			RNAPI void RemoveAllSegments();
			
			RNAPI void SetImageForSegmentAtIndex(Image *image, size_t index);
			RNAPI void SetTitleForSegmentAtIndex(String *title, size_t index);
			
			RNAPI void SetSegmentAtIndexEnabled(size_t index, bool enabled);
			RNAPI bool IsSegmentAtIndexEnabled(size_t index) const;
			
			RNAPI void SetRequiresSelection(bool requiresSelection);
			RNAPI void SetRequiresSingleSelection(bool singleSelection);
			
			RNAPI void SetFontForSegmentAtIndex(Font *font, size_t index);
			RNAPI void SetTextColorForSegmentAtIndex(Color *color, size_t index);
			
			RNAPI size_t GetNumberOfSegments() const { return _segments.GetCount(); }
			RNAPI size_t GetSelection() const;
			
			RNAPI void LayoutSubviews() override;
			
		private:
			void InsertSegment(Button *segment, size_t index);
			
			Array _segments;
			bool _requiresSelection;
			bool _singleSelection;
		};
	}
}

#endif /* __RAYNE_UISEGMENTVIEW_H__ */
