//
//  RNUIProgressIndicator.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIPROGRESSINDICATOR_H__
#define __RAYNE_UIPROGRESSINDICATOR_H__

#include "RNBase.h"
#include "RNProgress.h"
#include "RNUIView.h"
#include "RNUIImageView.h"

namespace RN
{
	namespace UI
	{
		class ProgressIndicator : public View
		{
		public:
			RNAPI ProgressIndicator();
			RNAPI ~ProgressIndicator();
			
			RNAPI void SetMin(double minValue);
			RNAPI void SetMax(double maxValue);
			RNAPI void SetProgress(double value);
			RNAPI void SetProgress(Progress *progress);
			
			RNAPI void Update() override;
			
		private:
			void UpdateToProgress(double value);
			void KickProgress();
			
			bool _dirty;
			
			ImageView *_background;
			ImageView *_bar;
			
			Progress *_progress;
			
			double _min;
			double _max;
			double _current;
			
			RNDeclareMeta(ProgressIndicator, View)
		};
	}
}

#endif /* __RAYNE_UIPROGRESSINDICATOR_H__ */
