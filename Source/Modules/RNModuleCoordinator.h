//
//  RNModuleCoordinator.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MODULECOORDINATOR_H_
#define __RAYNE_MODULECOORDINATOR_H_

#include "../Base/RNBase.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNDictionary.h"
#include "RNModule.h"

namespace RN
{
	class ModuleCoordinator
	{
	public:
		friend class Kernel;

		RN_OPTIONS(Options, uint32,
				   NoLoad = (1 << 0));

		static ModuleCoordinator *GetSharedInstance();

		RNAPI Module *GetModuleWithName(const String *name);
		RNAPI Module *GetModuleWithName(const String *name, Options options);

	private:
		ModuleCoordinator();
		~ModuleCoordinator();

		void LoadModules();

		std::mutex _lock;

		Array *_modules;
		Dictionary *_moduleMap;
	};
}


#endif /* __RAYNE_MODULECOORDINATOR_H_ */
