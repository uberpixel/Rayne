//
//  RNContext.h
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CONTEXT_H__
#define __RAYNE_CONTEXT_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNThread.h"

namespace RN
{
	class Context : public Object
	{
	public:
		virtual void MakeActiveContext();
		virtual void DeactiveContext();
		
		virtual void Flush() = 0;
		
		static Context *ActiveContext();
		
	private:
		bool _active;
		Thread *_thread;
	};
}

#endif /* __RAYNE_CONTEXT_H__ */
