//
//  RNModule.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../System/RNFileCoordinator.h"
#include "RNModule.h"

#if RN_PLATFORM_POSIX
#include <dlfcn.h>
#endif

namespace RN
{
	RNDefineMeta(Module, Object)

	Module::Module(const String *name) :
		_handle(nullptr),
		_ownsHandle(true),
		_name(SafeCopy(name)),
		_path(nullptr)
	{
		String *basePath = _name->StringByDeletingLastPathComponent();
		String *base = _name->GetLastPathComponent();

		base->Append("-x64"); // TODO: Don't hardcode this

#if RN_PLATFORM_MAC_OS
		base->AppendPathExtension(RNCSTR("dylib"));
#endif
#if RN_PLATFORM_WINDOWS
		base->AppendPathExtension(RNCSTR("dll"));
#endif
#if RN_PLATFORM_LINUX
		base->AppendPathExtension(RNCSTR("so"));
#endif

#if RN_PLATFORM_POSIX
		base->Insert(RNCSTR("lib"), 0);
#endif

		base = basePath->StringByAppendingPathComponent(base);


		_path = FileCoordinator::GetSharedInstance()->ResolveFullPath(base, 0);
		if(!_path)
			throw InvalidArgumentException("Couldn't resolve module name");

		_path->Retain();

		void *handle = dlopen(_path->GetUTF8String(), RTLD_GLOBAL | RTLD_NOLOAD);
		if(handle)
		{
			_handle = handle;
		}
		else
		{
			Catalogue::GetSharedInstance()->PushModule(this);
			_handle = dlopen(_path->GetUTF8String(), RTLD_NOW | RTLD_GLOBAL);
			Catalogue::GetSharedInstance()->PopModule();
		}

		if(!_handle)
			throw InvalidArgumentException(RNSTR(_name << " is not a valid dynamic library"));

		char buffer[512];
		strcpy(buffer, "__RN");
		strcat(buffer, name->GetLastPathComponent()->GetUTF8String());
		strcat(buffer, "Init");

		_initializer = reinterpret_cast<InitializeFunction>(dlsym(_handle, buffer));

		if(!_initializer)
			throw InvalidArgumentException(RNSTR(_name << " is not a valid dynamic library"));
	}

	Module::~Module()
	{
		if(_handle && _ownsHandle)
			dlclose(_handle);

		SafeRelease(_name);
		SafeRelease(_path);
	}

	void Module::Initialize()
	{
		Descriptor descriptor;
		memset(&descriptor, 0, sizeof(Descriptor));

		descriptor.module = this;

		if(!_initializer(&descriptor))
			throw InconsistencyException("Initializer failed");

		if(descriptor.abiVersion != GetABIVersion())
			throw InconsistencyException(RNSTR("Invalid ABI version reported by" << _name));
	}
}
