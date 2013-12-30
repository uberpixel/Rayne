//
//  RNUIScroller.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UISCROLLER_H__
#define __RAYNE_UISCROLLER_H__

#include "RNBase.h"
#include "RNUIView.h"
#include "RNUIImageView.h"
#include "RNUIButton.h"

namespace RN
{
	namespace UI
	{
		class ScrollView;
		class ScrollerKnob;
		class ScrollerFrame;
		
		class Scroller : public View
		{
		public:
			friend class ScrollView;
			
			RNAPI Scroller();
			RNAPI ~Scroller();
			
			RNAPI void LayoutSubviews() override;
			
			RNAPI float GetPreferredWidth() const { return _width; }
			
		private:
			void InsertIntoContainer(ScrollView *scrollView, bool horizontal);
			
			ScrollView *_container;
			
			ScrollerFrame *_frame;
			ScrollerKnob *_knob;
			
			bool _horizontal;
			float _width;
			
			RNDefineMeta(Scroller, View)
		};
	}
}

#endif /* __RAYNE_UISCROLLER_H__ */
