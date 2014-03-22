//
//  RNUIResponder.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIRESPONDER_H__
#define __RAYNE_UIRESPONDER_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNInput.h"

namespace RN
{
	namespace UI
	{
		class Responder : public Object
		{
		public:
			RNAPI virtual Responder *GetNextResponder() const;
			
			RNAPI virtual bool CanBecomeFirstResponder();
			RNAPI virtual bool CanResignFirstReponder();
			
			RNAPI virtual void BecomeFirstResponder();
			RNAPI virtual void ResignFirstResponder();
			
			RNAPI virtual void MouseDown(Event *event);
			RNAPI virtual void MouseUp(Event *event);
			RNAPI virtual void MouseMoved(Event *event);
			RNAPI virtual void MouseDragged(Event *event);
			RNAPI virtual void MouseLeft(Event *event);
			
			RNAPI virtual void KeyDown(Event *event);
			RNAPI virtual void KeyUp(Event *event);
			RNAPI virtual void KeyRepeat(Event *event);
			
			RNAPI virtual void ScrollWheel(Event *event);
			
		protected:
			Responder();
			~Responder() override;
		};
	}
}

#endif /* __RAYNE_UIRESPONDER_H__ */
