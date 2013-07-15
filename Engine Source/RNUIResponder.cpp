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
		Responder *Responder::_firstResponder = 0;
		
		Responder::Responder()
		{}
		
		Responder::~Responder()
		{
			if(_firstResponder == this)
			{
				_firstResponder = 0;
			}
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
			return (this == _firstResponder);
		}
		
		
		
		bool Responder::BecomeFirstResponder()
		{
			if(_firstResponder)
			{
				if(!_firstResponder->CanResignFirstReponder())
					return false;
				
				if(!_firstResponder->ResignFirstResponder())
					return false;
			}
			
			_firstResponder = this;
			return true;
		}
		
		bool Responder::ResignFirstResponder()
		{
			if(_firstResponder == this)
			{
				_firstResponder = 0;
				return true;
			}
			
			return false;
		}
		
		bool Responder::IsFirstResponder() const
		{
			return (this == _firstResponder);
		}
		
		
		
		void Responder::MouseDown(Event *event)
		{
			Responder *next = NextResponder();
			if(next)
				next->MouseDown(event);
		}
		
		void Responder::MouseUp(Event *event)
		{
			Responder *next = NextResponder();
			if(next)
				next->MouseUp(event);
		}
		
		void Responder::ScrollWheel(Event *event)
		{
			Responder *next = NextResponder();
			if(next)
				next->ScrollWheel(event);
		}
	}
}
