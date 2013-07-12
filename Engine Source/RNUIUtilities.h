//
//  RNUIUtilities.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE__UIUTILITIES_H__
#define __RAYNE__UIUTILITIES_H__

#include "RNBase.h"
#include "RNUIWidget.h"
#include "RNUILabel.h"
#include "RNRingbuffer.h"

namespace RN
{
	namespace UI
	{
		class DebugWidget : public Widget
		{
		public:
			DebugWidget();
			~DebugWidget() override;
			
		private:
			void HandleMessage(Message *message);
			
			float AverageFPS();
			
			Label *_fpsLabel;
			stl::ring_buffer<float> _fps;
		};
	}
}

#endif /* __RAYNE__UIUTILITIES_H__ */
