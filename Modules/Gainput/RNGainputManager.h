//
//  RNGainputManager.h
//  Rayne-Gainput
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_GAINPUTMANAGER_H_
#define __RAYNE_GAINPUTMANAGER_H_

#include "RNGainput.h"

namespace RN
{
	struct GainputManagerInternals;
	class GainputManager : public Object
	{
	public:
		GPAPI static GainputManager *GetInstance();

		GPAPI GainputManager();
		GPAPI ~GainputManager() override;
			
	private:
		static GainputManager *_instance;
		
		GainputManagerInternals *_internals;
			
		RNDeclareMetaAPI(GainputManager, GPAPI)
	};
}

#endif /* defined(__RAYNE_GAINPUTMANAGER_H_) */
