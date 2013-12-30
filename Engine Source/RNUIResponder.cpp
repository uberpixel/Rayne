//
//  RNUIResponder.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
		
		
		
		Responder *Responder::GetNextResponder() const
		{
			return nullptr;
		}
		
		
		
		bool Responder::CanBecomeFirstResponder()
		{
			return false;
		}
		
		bool Responder::CanResignFirstReponder()
		{
			return true;
		}
		
		
		
		void Responder::BecomeFirstResponder()
		{}
		
		void Responder::ResignFirstResponder()
		{}
		
		
		
		
		void Responder::MouseDown(Event *event)
		{
			Responder *next = GetNextResponder();
			if(next)
				next->MouseDown(event);
		}
		void Responder::MouseUp(Event *event)
		{
			Responder *next = GetNextResponder();
			if(next)
				next->MouseUp(event);
		}
		void Responder::MouseMoved(Event *event)
		{
			Responder *next = GetNextResponder();
			if(next)
				next->MouseMoved(event);
		}
		void Responder::MouseDragged(Event *event)
		{
			Responder *next = GetNextResponder();
			if(next)
				next->MouseDragged(event);
		}
		
		void Responder::KeyDown(Event *event)
		{
			Responder *next = GetNextResponder();
			if(next)
				next->KeyDown(event);
		}
		void Responder::KeyUp(Event *event)
		{
			Responder *next = GetNextResponder();
			if(next)
				next->KeyUp(event);
		}
		void Responder::KeyRepeat(Event *event)
		{
			Responder *next = GetNextResponder();
			if(next)
				next->KeyRepeat(event);
		}
		
		void Responder::ScrollWheel(Event *event)
		{
			Responder *next = GetNextResponder();
			if(next)
				next->ScrollWheel(event);
		}
	}
}
