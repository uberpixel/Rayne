//
//  RNModuleManager.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MODULEMANAGER_H_
#define __RAYNE_MODULEMANAGER_H_

#include "../Base/RNBase.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNDictionary.h"
#include "RNModule.h"

namespace RN
{
	class ModuleManager
	{
	public:
		friend class Kernel;
		friend class Catalogue;

		RN_OPTIONS(Options, uint32,
				   NoLoad = (1 << 0));

		RNAPI static ModuleManager *GetSharedInstance();

		RNAPI Module *GetModuleWithName(const String *name);
		RNAPI Module *GetModuleWithName(const String *name, Options options);

		RNAPI Module *GetModuleForClass(MetaClass *meta) const;

	private:
		ModuleManager();
		~ModuleManager();

		void LoadModules();

		Module *__GetModuleForSymbol(void *symbol);

		Lockable _lock;

		Array *_modules;
		Dictionary *_moduleMap;
	};
}


#endif /* __RAYNE_MODULEMANAGER_H_ */
