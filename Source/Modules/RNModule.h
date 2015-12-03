//
//  RNModule.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MODULE_H_
#define __RAYNE_MODULE_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNString.h"

namespace RN
{
	class Module : public Object
	{
	public:
		friend class ModuleManager;

		struct Descriptor
		{
			Module *module;

			char identifier[512];
			uint32 abiVersion;
		};

		typedef bool (*InitializeFunction)(Descriptor *descriptor);

		const String *GetName() const { return _name; }
		const String *GetPath() const { return _path; }
		const String *GetIdentifier() const { return _identifier; }

		RNAPI const String *GetDescription() const override;
		RNAPI const String *GetPathForResource(const String *resource);

	private:
		Module(const String *name);
		~Module();

		void Initialize();

		bool _ownsHandle;

#if RN_PLATFORM_POSIX
		void *_handle;
#endif
#if RN_PLATFORM_WINDOWS
		HMODULE _handle;
#endif

		String *_name;
		String *_identifier;

		String *_path;
		String *_resourcePath;
		String *_lookupPrefix;

		InitializeFunction _initializer;

		RNDeclareMeta(Module)
	};
}

#define RNModule(name, _identifier) \
	extern "C" bool __RN##name##Init(RN::Module::Descriptor *descriptor); \
	bool __RN##name##Init(RN::Module::Descriptor *descriptor) { \
		strcpy(descriptor->identifier, _identifier); \
		descriptor->abiVersion = kRNABIVersion; \
		return true; \
	}

#endif /* __RAYNE_MODULE_H_ */
