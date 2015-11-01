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
		_path(nullptr),
		_identifier(nullptr)
	{
		String *basePath = _name->StringByDeletingLastPathComponent();
		String *base = _name->GetLastPathComponent();

#if RN_PLATFORM_64BIT
		base->Append("-x64");
#endif
#if RN_PLATFORM_32BIT
		base->Append("-x86");
#endif


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

		char buffer[512];
		strcpy(buffer, "__RN");
		strcat(buffer, name->GetLastPathComponent()->GetUTF8String());
		strcat(buffer, "Init");

		void *initializer = nullptr;

		if((initializer = dlsym(RTLD_DEFAULT, buffer)))
		{
			_handle = nullptr;
			_ownsHandle = false;

			Dl_info info;
			int status = dladdr(initializer, &info);
			if(status != 0)
			{
				_path->Release();
				_path = RNSTR(info.dli_fname)->Retain();

				_handle = dlopen(info.dli_fname, RTLD_NOLOAD | RTLD_GLOBAL);
				_ownsHandle = true;
			}
		}
		else
		{
			Catalogue::GetSharedInstance()->PushModule(this);
			_handle = dlopen(_path->GetUTF8String(), RTLD_NOW | RTLD_GLOBAL);
			Catalogue::GetSharedInstance()->PopModule();
		}

		if(!_handle && _ownsHandle)
			throw InvalidArgumentException(RNSTR(_name << " is not a valid dynamic library"));

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
		SafeRelease(_identifier);
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

		_identifier = new String(descriptor.identifier, Encoding::UTF8, false);
	}

	const String *Module::GetDescription() const
	{
		return RNSTR("<RN::Module:" << (void *)this << " " << GetIdentifier() << ">");
	}
}
