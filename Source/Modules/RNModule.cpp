//
//  RNModule.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../System/RNFileManager.h"
#include "RNModule.h"
#include "../Debug/RNLogger.h"

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
		FileManager *coordinator = FileManager::GetSharedInstance();
		
#if RN_PLATFORM_IOS || RN_PLATFORM_VISIONOS
		RN::String *frameworkName = _name->GetPathComponents()->GetLastObject<RN::String>();
		RN::String *libraryNameIOS = frameworkName;
		//TODO: Library files may also have no extensions, so this check is kinda bad...
		if(!frameworkName->GetPathExtension())
		{
			frameworkName = RNSTR(coordinator->GetPathForLocation(FileManager::Location::ApplicationDirectory) << "/Frameworks/" << frameworkName << ".framework");
		}
		else
		{
			frameworkName = RNSTR(coordinator->GetPathForLocation(FileManager::Location::ApplicationDirectory) << "/Frameworks/" << frameworkName);
		}
		SafeRelease(_name);
		_name = SafeRetain(frameworkName);
#endif

		FileManager::Node *node = coordinator->ResolvePath(_name, 0);
		if(node)
		{
			if(node->GetType() == FileManager::Node::Type::Directory)
			{
				isDirectory = true;
			}
		}
		else
		{
			String *path = coordinator->ResolveFullPath(_name, 0);
			if(path)
			{
				coordinator->PathExists(path, isDirectory);
			}
		}

		if(isDirectory)
		{
#if RN_PLATFORM_IOS || RN_PLATFORM_VISIONOS
			_name->AppendPathComponent(libraryNameIOS);
#else
			_name->AppendPathComponent(_name->GetLastPathComponent());
#endif
		}

		// Resolve the files
		String *basePath = _name->StringByDeletingLastPathComponent();
		String *base = _name->GetLastPathComponent();

#if RN_PLATFORM_MAC_OS
		base->AppendPathExtension(RNCSTR("dylib"));
#endif
#if RN_PLATFORM_WINDOWS
		base->AppendPathExtension(RNCSTR("dll"));
#endif
#if RN_PLATFORM_LINUX || RN_PLATFORM_ANDROID
		base->AppendPathExtension(RNCSTR("so"));
#endif

		Array *paths = new Array();

		paths->AddObject(basePath->StringByAppendingPathComponent(base));
		paths->AddObject(coordinator->GetPathForLocation(FileManager::Location::ApplicationDirectory)->StringByAppendingPathComponent(base));
		base->Insert(RNCSTR("lib"), 0);
		paths->AddObject(basePath->StringByAppendingPathComponent(base));
		paths->AddObject(coordinator->GetPathForLocation(FileManager::Location::ApplicationDirectory)->StringByAppendingPathComponent(base));

		paths->Enumerate<String>([&](String *path, size_t index, bool &stop) {

			_path = coordinator->ResolveFullPath(path, 0);
			if(_path)
				stop = true;

		});
		paths->Release();

#if RN_PLATFORM_ANDROID
		//Android modules are not inside the resource directory, only the module resources can be found there.
		//Instead libraries get automatically unpacked and loaded on startup.
		//By using the name directly this code will figure that out further down.
		if(!_path)
		{
			_path = _name;
		}
#endif

		if(!_path)
			throw InvalidArgumentException(RNSTR("Couldn't resolve module name: " << name));

		_path->Retain();
		_name = _name->GetLastPathComponent();

		// Resolve extra paths
		if(isDirectory)
		{
#if RN_PLATFORM_IOS || RN_PLATFORM_VISIONOS
			String *relativeResourcePath = basePath->StringByAppendingPathComponent(RNCSTR("ResourceFiles/Resources"));
#else
			String *relativeResourcePath = basePath->StringByAppendingPathComponent(RNCSTR("Resources"));
#endif
			_resourcePath = SafeRetain(coordinator->ResolveFullPath(relativeResourcePath, 0));

			if(_resourcePath && !coordinator->PathExists(_resourcePath))
				SafeRetain(_resourcePath);

			if(_resourcePath)
			{
				_lookupPrefix = _name->Copy();
				_lookupPrefix->Insert(RNCSTR(":"), 0);
				_lookupPrefix->Append(RNCSTR(":"));

				coordinator->__AddModuleWithPath(this, _resourcePath);
			}
		}

		String *error = nullptr;
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
					RNDebug(RNSTR("Already loaded library: ") << _path);

					int flags = RTLD_GLOBAL;

#if RN_PLATFORM_MAC_OS || RN_PLATFORM_IOS || RN_PLATFORM_VISIONOS
					flags |= RTLD_NOLOAD;
#else
					flags |= RTLD_NOW;
#endif

					_handle = dlopen(info.dli_fname, flags);
					_ownsHandle = true;

					if(!_handle)
					{
						error = RNSTR(dlerror());
					}
				}
				else
				{
					error = RNSTR("Found " << buffer << "() already loaded, but couldn't find the loaded library");
				}
			}
			else
			{
				Catalogue::GetSharedInstance()->PushModule(this);
				_handle = dlopen(_path->GetUTF8String(), RTLD_NOW | RTLD_GLOBAL);
				Catalogue::GetSharedInstance()->PopModule();

				if(!_handle)
					error = RNSTR(dlerror());
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
			{
				String *appendix = RNCSTR("");

				if(error)
					appendix = RNSTR(" Error: " << error);

				throw InvalidArgumentException(RNSTR(_name << " is not a valid dynamic library." << appendix));
			}

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
			FileManager::GetSharedInstance()->__RemoveModule(this);
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

		FileManager::GetSharedInstance()->__RemoveModule(this);
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
			void *result = reinterpret_cast<void *>(::GetProcAddress(modules[i], name));
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
			FileManager::GetSharedInstance()->__RemoveModule(this);
			std::rethrow_exception(std::current_exception());
		}

		_identifier = new String(descriptor.identifier, Encoding::UTF8, false);

		// Since we just loaded a new module, we might be able to link meta classes with their modules
		Catalogue::GetSharedInstance()->DoClassesPreFlight();
	}

	const String *Module::GetDescription() const
	{
#if RN_PLATFORM_ANDROID
		const String *path = GetPath();
		//The below will try to open the library file on android, but then fails and throws an exception due to the whole unzipping thing
#else
		const String *path = FileManager::GetSharedInstance()->GetNormalizedPathFromFullPath(GetPath());
		if(!path)
			path = GetPath();
#endif

		return RNSTR("<RN::Module:" << (void *)this << "> (" << GetName() << ", " << path << ")");
	}

	const String *Module::GetPathForResource(const String *resource)
	{
		if(!_resourcePath)
			return nullptr;

		String *path = _lookupPrefix->StringByAppendingPathComponent(resource);
		return FileManager::GetSharedInstance()->ResolveFullPath(path, 0);
	}
}
