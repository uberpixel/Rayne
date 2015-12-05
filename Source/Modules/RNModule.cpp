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
		_ownsHandle(true),
		_handle(nullptr),
		_name(SafeCopy(name)),
		_identifier(nullptr),
		_path(nullptr),
		_resourcePath(nullptr),
		_lookupPrefix(nullptr)
	{
		// Figure out if name points to a folder
		bool isDirectory = false;
		FileCoordinator *coordinator = FileCoordinator::GetSharedInstance();

		String *path = coordinator->ResolveFullPath(_name, 0);
		if(path)
		{
			if(coordinator->PathExists(path, isDirectory) && isDirectory)
				_name->AppendPathComponent(_name->GetLastPathComponent());
		}


		// Resolve the files
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


		_path = coordinator->ResolveFullPath(base, 0);
		if(!_path)
			throw InvalidArgumentException(RNSTR("Couldn't resolve module name: " << name));

		_path->Retain();
		_name = _name->GetLastPathComponent();

		// Resolve extra paths
		if(isDirectory)
		{
			String *basePath = _path->StringByDeletingLastPathComponent();

			_resourcePath = SafeRetain(basePath->StringByAppendingPathComponent(RNCSTR("Resources")));

			if(!coordinator->PathExists(_resourcePath))
				_resourcePath = nullptr;

			if(_resourcePath)
			{
				_lookupPrefix = _name->Copy();
				_lookupPrefix->Insert(RNCSTR(":"), 0);
				_lookupPrefix->Append(RNCSTR(":"));

				coordinator->__AddModuleWithPath(this, _resourcePath);
			}
		}

		try
		{
			// Load the library
			char buffer[512];
			strcpy(buffer, "__RN");
			strcat(buffer, name->GetLastPathComponent()->GetUTF8String());
			strcat(buffer, "Init");

			void *initializer = nullptr;

#if RN_PLATFORM_POSIX
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
#endif
#if RN_PLATFORM_WINDOWS
			HMODULE module = GetModuleWithFunction(buffer);
			if(module)
			{
				char pathBuffer[1024];

				size_t result = ::GetModuleFileName(module, pathBuffer, 1024);
				pathBuffer[result + 1] = '\0';

				_handle = module;
				_path = RNSTR(pathBuffer)->StringByAppendingPathComponent(base)->Retain();
				_ownsHandle = false;
			}
			else
			{
				Catalogue::GetSharedInstance()->PushModule(this);
				_ownsHandle = true;
				_handle = ::LoadLibrary(_path->GetUTF8String());
				Catalogue::GetSharedInstance()->PopModule();
			}
#endif

			if(!_handle && _ownsHandle)
				throw InvalidArgumentException(RNSTR(_name << " is not a valid dynamic library"));

#if RN_PLATFORM_POSIX
			_initializer = reinterpret_cast<InitializeFunction>(dlsym(_handle, buffer));
#endif
#if RN_PLATFORM_WINDOWS
			_initializer = reinterpret_cast<InitializeFunction>(::GetProcAddress(_handle, buffer));
#endif

			if(!_initializer)
				throw InvalidArgumentException(RNSTR(_name << " is not a valid dynamic library"));
		}
		catch(...)
		{
			FileCoordinator::GetSharedInstance()->__RemoveModule(this);
			std::rethrow_exception(std::current_exception());
		}
	}

	Module::~Module()
	{
#if RN_PLATFORM_POSIX
		if(_handle && _ownsHandle)
			dlclose(_handle);
#endif
#if RN_PLATFORM_WINDOWS
		if(_handle && _ownsHandle)
			FreeLibrary(_handle);
#endif

		SafeRelease(_name);
		SafeRelease(_path);
		SafeRelease(_identifier);
		SafeRelease(_resourcePath);
		SafeRelease(_lookupPrefix);

		FileCoordinator::GetSharedInstance()->__RemoveModule(this);
	}

#if RN_PLATFORM_WINDOWS
	HMODULE Module::GetModuleWithFunction(const char *name)
	{
		HANDLE process = ::GetCurrentProcess();
		DWORD needed;

		::EnumProcessModulesEx(process, NULL, 0, &needed, LIST_MODULES_ALL);

		size_t count = needed / sizeof(HMODULE);

		HMODULE *modules = new HMODULE[count];
		::EnumProcessModulesEx(process, modules, needed, &needed, LIST_MODULES_ALL);

		for(size_t i = 0; i < count; i++)
		{
			void *result = ::GetProcAddress(modules[i], name);
			if(result)
			{
				HMODULE result = modules[i];

				delete[] modules;
				return result;
			}
		}

		delete[] modules;
		return 0;
	}
#endif

	void Module::Initialize()
	{
		Descriptor descriptor;
		memset(&descriptor, 0, sizeof(Descriptor));

		descriptor.module = this;

		try
		{
			if(!_initializer(&descriptor))
				throw InconsistencyException("Initializer failed");

			if(descriptor.abiVersion != GetABIVersion())
				throw InconsistencyException(RNSTR("Invalid ABI version reported by" << _name));
		}
		catch(...)
		{
			FileCoordinator::GetSharedInstance()->__RemoveModule(this);
			std::rethrow_exception(std::current_exception());
		}

		_identifier = new String(descriptor.identifier, Encoding::UTF8, false);

		// Since we just loaded a new module, we might be able to link meta classes with their modules
		Catalogue::GetSharedInstance()->DoClassesPreFlight();
	}

	const String *Module::GetDescription() const
	{
		return RNSTR("<RN::Module:" << (void *)this << " " << GetIdentifier() << ">");
	}

	const String *Module::GetPathForResource(const String *resource)
	{
		if(!_resourcePath)
			return nullptr;

		String *path = _lookupPrefix->StringByAppendingPathComponent(resource);
		return FileCoordinator::GetSharedInstance()->ResolveFullPath(path, 0);
	}
}
