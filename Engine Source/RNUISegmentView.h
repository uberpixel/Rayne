//
//  RNUISegmentView.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
			SegmentView();
			~SegmentView() override;
			
			void InsertegmentAtIndex(Image *image, size_t index);
			void InsertSegmentAtIndex(String *title, size_t index);
			
			void RemoveSegmentAtIndex(size_t index);
			void RemoveAllSegments();
			
			void SetImageForSegmentAtIndex(Image *image, size_t index);
			void SetTitleForSegmentAtIndex(String *title, size_t index);
			
			void SetSegmentAtIndexEnabled(size_t index, bool enabled);
			bool IsSegmentAtIndexEnabled(size_t index) const;
			
			size_t GetNumberOfSegments() const { return _segments.GetCount(); }
			
			void LayoutSubviews() override;
			
		private:
			void InsertSegment(Button *segment, size_t index);
			Array _segments;
		};
	}
}

#endif /* __RAYNE_UISEGMENTVIEW_H__ */
