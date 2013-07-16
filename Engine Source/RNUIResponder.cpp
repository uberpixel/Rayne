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
			return true;
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
			
			if(CanBecomeFirstResponder())
			{
				printf("Became first responder");
				
				_firstResponder = this;
				return true;
			}
			
			return false;
		}
		
		bool Responder::ResignFirstResponder()
		{
			if(_firstResponder == this)
			{
				if(!CanResignFirstReponder())
					return false;
				
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
		void Responder::MouseMoved(Event *event)
		{
			Responder *next = NextResponder();
			if(next)
				next->MouseMoved(event);
		}
		
		void Responder::KeyDown(Event *event)
		{
			Responder *next = NextResponder();
			if(next)
				next->KeyDown(event);
		}
		void Responder::KeyUp(Event *event)
		{
			Responder *next = NextResponder();
			if(next)
				next->KeyUp(event);
		}
		void Responder::KeyRepeat(Event *event)
		{
			Responder *next = NextResponder();
			if(next)
				next->KeyRepeat(event);
		}
		
		void Responder::ScrollWheel(Event *event)
		{
			Responder *next = NextResponder();
			if(next)
				next->ScrollWheel(event);
		}
	}
}
