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
		friend class Catalogue;

		RN_OPTIONS(Options, uint32,
				   NoLoad = (1 << 0));

		RNAPI static ModuleCoordinator *GetSharedInstance();

		RNAPI Module *GetModuleWithName(const String *name);
		RNAPI Module *GetModuleWithName(const String *name, Options options);

		RNAPI Module *GetModuleForClass(MetaClass *meta) const;

	private:
		ModuleCoordinator();
		~ModuleCoordinator();

		void LoadModules();

		Module *__GetModuleForSymbol(void *symbol);

		std::mutex _lock;

		Array *_modules;
		Dictionary *_moduleMap;
	};
}


#endif /* __RAYNE_MODULECOORDINATOR_H_ */
