//
//  RNUIResponder.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIResponder.h"

namespace RN
{
	namespace UI
	{
		static Responder *__FirstResponder = 0;
		
		Responder::Responder()
		{
		}
		
		Responder::~Responder()
		{
		}
		
		
		
		Responder *Responder::NextResponder() const
		{
			return 0;
		}
		
		
		bool Responder::CanBecomeFirstResponder() const
		{
			return false;
		}
		
		bool Responder::CanResignFirstReponder() const
		{
			return (this == __FirstResponder);
		}
		
		
		bool Responder::BecomeFirstResponder()
		{
			if(__FirstResponder)
			{
				if(!__FirstResponder->CanResignFirstReponder())
					return false;
				
				if(!__FirstResponder->ResignFirstResponder())
					return false;
			}
			
			__FirstResponder = this;
			return true;
		}
		
		bool Responder::ResignFirstResponder()
		{
			if(this == __FirstResponder)
			{
				__FirstResponder = 0;
				return true;
			}
			
			return false;
		}
		
		
		
		bool Responder::IsFirstResponder() const
		{
			return (this == __FirstResponder);
		}
		
		Responder *Responder::FirstResponder()
		{
			return __FirstResponder;
		}
	}
}
