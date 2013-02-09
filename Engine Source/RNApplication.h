//
//  RNApplication.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_APPLICATION_H__
#define __RAYNE_APPLICATION_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Application : public UnconstructingSingleton<Application>
	{
	public:
		Application();
		virtual ~Application();
		
		virtual void Start();
		virtual bool CanExit();
		virtual void WillExit();
		
		virtual void GameUpdate(float delta);
		virtual void WorldUpdate(float delta);
	};
}

#endif /* __RAYNE_APPLICATION_H__ */
