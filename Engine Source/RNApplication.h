//
//  RNApplication.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_APPLICATION_H__
#define __RAYNE_APPLICATION_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Application : public INonConstructingSingleton<Application>
	{
	public:
		RNAPI Application();
		RNAPI virtual ~Application();
		
		RNAPI virtual void Start();
		RNAPI virtual void WillExit();
		
		RNAPI virtual void GameUpdate(float delta);
		RNAPI virtual void WorldUpdate(float delta);
		
		RNAPI void SetTitle(const std::string& title);
		const std::string& Title() const { return _title; }
		
	protected:
		std::string _title;
		
		RNDefineSingleton(Application)
	};
}

#endif /* __RAYNE_APPLICATION_H__ */
