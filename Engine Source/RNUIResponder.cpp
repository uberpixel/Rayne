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
		Responder::Responder()
		{}
		
		Responder::~Responder()
		{}
		
		
		
		Responder *Responder::NextResponder() const
		{
			return nullptr;
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
			return CanBecomeFirstResponder();
		}
		
		bool Responder::ResignFirstResponder()
		{
			return CanResignFirstReponder();
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
