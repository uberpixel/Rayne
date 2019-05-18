//
//  RNFileManager.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Base/RNBaseInternal.h"
#include "../Base/RNKernel.h"
#include "../Base/RNApplication.h"
#include "../Modules/RNModule.h"
#include "RNFileManager.h"

#if RN_PLATFORM_POSIX
	#include <dirent.h>
	#include <unistd.h>
	#include <sys/stat.h>
#endif
#if RN_PLATFORM_WINDOWS
	#include <io.h>
#endif

#if RN_COMPILER_MSVC
	#define F_OK 0
#endif

namespace RN
{
	RNDefineScopedMeta(FileManager, Node, Object)
	RNDefineScopedMeta(FileManager, Directory, FileManager::Node)
	RNDefineScopedMeta(FileManager, File, FileManager::Node)

	const String *_platformModifier;

	FileManager::Node::Node(String *name, Node *parent, Type type) :
		_type(type),
		_name(name->Retain()),
		_path(nullptr),
		_modifier(nullptr),
		_parent(parent)
	{
		if(_parent)
			SetPath(_parent->GetPath()->StringByAppendingPathComponent(name));

		Range range = name->GetRangeOfString(RNCSTR("~"), String::ComparisonMode::Reverse);
		if(range.origin != kRNNotFound)
		{
			String *extension = name->GetPathExtension();

			size_t modifierLength = _name->GetLength() - range.origin - extension->GetLength() - 1;

			_modifier = _name->GetSubstring(Range(range.origin, modifierLength))->Retain();
			_name->DeleteCharacters(Range(range.origin, modifierLength));
		}
	}
	FileManager::Node::~Node()
	{
		SafeRelease(_path);
		SafeRelease(_modifier);
		_name->Release();
	}

	void FileManager::Node::SetPath(String *path)
	{
		SafeRelease(_path);
		_path = SafeRetain(path);
	}


	FileManager::Directory::Directory(String *name, Node *parent) :
		Node(name, parent, Type::Directory),
		_children(new Array()),
		_childMap(new Dictionary())
	{
		ParseDirectory();
	}

	FileManager::Directory::Directory(const String *path) :
		Node(path->GetLastPathComponent(), nullptr, Type::Directory),
		_children(new Array()),
		_childMap(new Dictionary())
	{
		SetPath(path->Copy()->Autorelease());
		ParseDirectory();
	}

	FileManager::Directory::~Directory()
	{
		_children->Release();
		_childMap->Release();
	}

	FileManager::Node *FileManager::Directory::GetChildWithName(const String *name) const
	{
		return _childMap->GetObjectForKey<FileManager::Node>(name);
	}

	void FileManager::Directory::ParseDirectory()
	{
#if RN_PLATFORM_ANDROID
		int errorAndroid = errno;

		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		AAssetDir *dir = AAssetManager_openDir(app->activity->assetManager, GetPath()->GetUTF8String());
		if(!dir)
		{
			errno = errorAndroid;
			throw InconsistencyException(RNSTR("Couldn't open directory " << GetPath()));
		}

		const char *assetName;
		while((assetName = AAssetDir_getNextFileName(dir)))
		{
			if(assetName[0] != '\0' && assetName[0] != '.')
			{
				RNDebug("Asset name: " << assetName);

				Node *node = nullptr;
				node = new File(RNSTR(assetName), this);

				if(node)
				{
					// Only allow nodes matching the platform modifier, ie ~osx or ~win
					if(node->GetModifier())
					{
						if(!node->GetModifier()->IsEqual(_platformModifier))
						{
							node->Release();
							continue;
						}
					}

					// Make sure only the platform modifier node is allowed into the VFS,
					// the regular version is removed if necessary
					Node *other = _childMap->GetObjectForKey<Node>(node->GetName());
					if(other)
					{
						if(!node->GetModifier())
						{
							node->Release();
							continue;
						}

						_children->RemoveObject(other);
					}

					_children->AddObject(node);
					_childMap->SetObjectForKey(node, node->GetName());

					node->Release();
				}
			}
		}

		AAssetDir_close(dir);
		return;

#elif RN_PLATFORM_POSIX
		int error = errno;
		DIR *dir = opendir(GetPath()->GetUTF8String());

		if(!dir)
		{
			errno = error;
			throw InconsistencyException(RNSTR("Couldn't open directory " << GetPath()));
		}

		struct dirent *ent;
		while((ent = readdir(dir)))
		{
			if(ent->d_name[0] != '\0' && ent->d_name[0] != '.')
			{
				Node *node = nullptr;

				switch(ent->d_type)
				{
					case DT_DIR:
						node = new Directory(RNSTR(ent->d_name), this);
						break;

					case DT_REG:
						node = new File(RNSTR(ent->d_name), this);
						break;

					default:
						break;
				}

				if(node)
				{
					// Only allow nodes matching the platform modifier, ie ~osx or ~win
					if(node->GetModifier())
					{
						if(!node->GetModifier()->IsEqual(_platformModifier))
						{
							node->Release();
							continue;
						}
					}

					// Make sure only the platform modifier node is allowed into the VFS,
					// the regular version is removed if necessary
					Node *other = _childMap->GetObjectForKey<Node>(node->GetName());
					if(other)
					{
						if(!node->GetModifier())
						{
							node->Release();
							continue;
						}

						_children->RemoveObject(other);
					}

					_children->AddObject(node);
					_childMap->SetObjectForKey(node, node->GetName());

					node->Release();
				}
			}
		}

		closedir(dir);
#endif

#if RN_PLATFORM_WINDOWS
		WIN32_FIND_DATA ffd;

		std::stringstream stream;
		stream << GetPath()->GetUTF8String() << "\\*";

		HANDLE handle = ::FindFirstFile(stream.str().c_str(), &ffd);

		if(handle == INVALID_HANDLE_VALUE)
			return;

		do {

			if(ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
				continue;

			if(strlen(ffd.cFileName) > 0 && ffd.cFileName[0] != '.')
			{
				Node *node = nullptr;

				if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					node = new Directory(RNSTR(ffd.cFileName), this);
				}
				else
				{
					node = new File(RNSTR(ffd.cFileName), this);
				}

				if(node)
				{
					// Only allow nodes matching the platform modifier, ie ~osx or ~win
					if(node->GetModifier())
					{
						if(!node->GetModifier()->IsEqual(_platformModifier))
						{
							node->Release();
							continue;
						}
					}

					// Make sure only the platform modifier node is allowed into the VFS,
					// the regular version is removed if necessary
					Node *other = _childMap->GetObjectForKey<Node>(node->GetName());
					if(other)
					{
						if(!node->GetModifier())
						{
							node->Release();
							continue;
						}

						_children->RemoveObject(other);
					}

					_children->AddObject(node);
					_childMap->SetObjectForKey(node, node->GetName());

					node->Release();
				}
			}

		} while(::FindNextFile(handle, &ffd));

		::FindClose(handle);

#endif
	}

	FileManager::File::File(String *name, Node *parent) :
		Node(name, parent, Type::File)
	{}

	// ---------------------
	// MARK: -
	// MARK: FileManager
	// ---------------------

#if RN_PLATFORM_POSIX
	char *realpath_expand(const char *path, char *buffer)
	{
#if RN_PLATFORM_LINUX || RN_PLATFORM_ANDROID
		char *home;

		if(path[0] == '~' && (home = getenv("HOME")))
		{
			char temp[PATH_MAX];
			return realpath(strcat(strcpy(temp, home), path + 1), buffer);
		}
		else
		{
			return realpath(path, buffer);
		}
#endif
#if RN_PLATFORM_MAC_OS
		if(path[0] == '~')
		{
			NSString *string = [[NSString stringWithCString:path encoding:NSASCIIStringEncoding] stringByExpandingTildeInPath];
			return realpath([string UTF8String], buffer);
		}
		else
		{
			return realpath(path, buffer);
		}
#endif
	}
#endif

	static FileManager *__sharedInstance = nullptr;

	FileManager::FileManager() :
		_nodes(new Array()),
		_modulePaths(new Dictionary())
	{
		__sharedInstance = this;

#if RN_PLATFORM_MAC_OS
		_platformModifier = RNCSTR("~macos")->Retain();
#endif
#if RN_PLATFORM_WINDOWS
		_platformModifier = RNCSTR("~windows")->Retain();
#endif
#if RN_PLATFORM_LINUX
		_platformModifier = RNCSTR("~linux")->Retain();
#endif
#if RN_PLATFORM_ANDROID
		_platformModifier = RNCSTR("~android")->Retain();
#endif
	}
	FileManager::~FileManager()
	{
		SafeRelease(_nodes);
		SafeRelease(_modulePaths);

		__sharedInstance = nullptr;
	}
	FileManager *FileManager::GetSharedInstance()
	{
		return __sharedInstance;
	}

	void FileManager::__PrepareWithManifest()
	{
		Array *paths = Kernel::GetSharedInstance()->GetManifestEntryForKey<Array>(kRNManifestSearchPathsKey);
		if(paths)
		{
			String *delimiter = RNCSTR("./");
			String *base = GetPathForLocation(Location::ApplicationDirectory);

			paths->Enumerate<String>([&](String *path, size_t index, bool &stop) {

				Range range = path->GetRangeOfString(delimiter, 0, Range(0, 2));
				if(range.origin == 0)
				{
					path = path->GetSubstring(Range(1, path->GetLength() - 1));
					path->Insert(base, 0);
				}

				AddSearchPath(path);

			});
		}
	}

	String *FileManager::__ExpandPath(const String *tpath)
	{
#if RN_PLATFORM_POSIX
		char buffer[PATH_MAX];

		int error = errno;
		char *cpath = tpath->GetUTF8String();

		char *result = realpath_expand(cpath, buffer);
		String *path = String::WithString(result ? buffer : cpath);

		if(!result)
			errno = error;
#else
		char buffer[MAX_PATH];

		auto cpath = tpath->GetUTF8String();
		DWORD result = ::GetFullPathNameA(cpath, MAX_PATH, buffer, nullptr);
		String *path = String::WithString(((result != 0) ? buffer : cpath));
#endif

		return path;
	}

	String *FileManager::GetNormalizedPathFromFullPath(const String *fullPath)
	{
#if RN_PLATFORM_ANDROID
		return RNSTR(fullPath);
#endif

		char buffer[1024];
#if RN_PLATFORM_POSIX
		int error = errno;
		char *result = realpath_expand(fullPath->GetUTF8String(), buffer);

		if(!result)
		{
			errno = error;
			throw InvalidArgumentException(RNSTR("No such file or directory " << fullPath));
		}
#else
		DWORD result = ::GetFullPathNameA(fullPath->GetUTF8String(), 1024, buffer, nullptr);
		if(result == 0)
			throw InvalidArgumentException(RNSTR("No such file or directory " << fullPath));
#endif

		String *path = RNSTR(buffer);

		LockGuard<Lockable> lock(_lock);

		_nodes->Enumerate<Directory>([&](Directory *directory, size_t tindex, bool &stop) {

			if(path->HasPrefix(directory->GetPath()))
			{
				size_t length = directory->GetPath()->GetLength() + 1;

				path = path->GetSubstring(Range(length, path->GetLength() - length));
				stop = true;
			}

		});

		return path;
	}

	Array *FileManager::__GetNodeContainerForPath(const String *path, Array *&outPath)
	{
		Array *components = path->GetPathComponents();
		if(components->GetCount() > 1)
		{
			String *component = components->GetObjectAtIndex<String>(0);
			Array *nodes = _modulePaths->GetObjectForKey<Array>(component);

			if(nodes)
			{
				components->RemoveObjectAtIndex(0);
				outPath = components;

				return nodes;
			}
		}

		outPath = components;
		return _nodes;
	}

	FileManager::Node *FileManager::ResolvePath(const String *path, ResolveHint hint) RN_NOEXCEPT
	{
		LockGuard<Lockable> lock(_lock);

		Array *components = nullptr;

		Node *result = nullptr;
		Array *nodes = __GetNodeContainerForPath(path, components);

		RN_ASSERT(components, "outPath is null!"); // Should _NEVER_ happen
		size_t componentCount = components->GetCount();

		if(RN_EXPECT_FALSE(componentCount == 0))
			return nullptr;

		nodes->Enumerate<Directory>([&](Directory *directory, size_t index, bool &stop) {

			Directory *node = directory;

			for(size_t i = 0; i < componentCount; i++)
			{
				String *component = components->GetObjectAtIndex<String>(i);
				Node *subnode = node->GetChildWithName(component);

				bool lastNode = (i == componentCount - 1);

				if(!subnode)
				{
					if(lastNode && ((hint & ResolveHint::CreateNode) || node->GetName()->IsEqual(component)))
						result = node;

					break;
				}

				if(lastNode)
				{
					result = subnode;
					break;
				}

				if(subnode->GetType() == Node::Type::File)
					break;

				node = static_cast<Directory *>(subnode);
			}

			if(result)
				stop = true;

		});

		if(result)
			return result->Retain()->Autorelease();

		return nullptr;
	}

	String *FileManager::ResolveFullPath(const String *path, ResolveHint hint) RN_NOEXCEPT
	{
		Node *result = ResolvePath(path, hint);
		if(result)
		{
			if(hint & ResolveHint::CreateNode)
			{
				String *last = path->GetLastPathComponent();

				if(result->GetName()->IsEqual(last))
					return result->GetPath()->Copy()->Autorelease();

				return result->GetPath()->StringByAppendingPathComponent(last);
			}

			return result->GetPath()->GetNormalizedPath();
		}
		
		String *expanded = __ExpandPath(path);

		if(access(expanded->GetUTF8String(), F_OK) != -1)
			return expanded->GetNormalizedPath();

		if(hint & ResolveHint::CreateNode)
		{
			String *parent = expanded->StringByDeletingLastPathComponent();
			
			if(access(parent->GetUTF8String(), F_OK) != -1)
				return expanded->GetNormalizedPath();
		}

		return nullptr;
	}

	String *FileManager::GetPathForLocation(Location location) const
	{
#if RN_PLATFORM_MAC_OS
		static bool isAppBundle = false;
		static std::once_flag flag;

		std::call_once(flag, [&]{

			NSArray *arguments = [[NSProcessInfo processInfo] arguments];
			NSString *directory = [arguments objectAtIndex:0];

			isAppBundle = ([directory rangeOfString:@".app"].location != NSNotFound);

		});
#endif
		switch(location)
		{
			case Location::ApplicationDirectory:
			{
#if RN_PLATFORM_MAC_OS
				if(isAppBundle)
				{
					NSBundle *bundle = [NSBundle mainBundle];
					NSString *path = [bundle bundlePath];

					return RNSTR([path UTF8String]);
				}
				else
				{
					NSArray *arguments = [[NSProcessInfo processInfo] arguments];
					NSString *directory = [arguments objectAtIndex:0];

					String *result = RNSTR([directory UTF8String]);
					result = result->StringByDeletingLastPathComponent();

					return result;
				}
#endif
#if RN_PLATFORM_WINDOWS
				char buffer[MAX_PATH];
				DWORD size = MAX_PATH;

				::GetModuleFileNameA(0, buffer, size);

				char *temp = buffer + strlen(buffer);
				while(temp != buffer)
				{
					temp --;

					if(*temp == '/' || *temp == '\\')
					{
						*temp = '\0';
						break;
					}
				}

				return RNSTR(buffer);
#endif
#if RN_PLATFORM_LINUX
				char buffer[PATH_MAX];
				size_t size = PATH_MAX;
				readlink("/proc/self/exe", buffer, size);

				char *temp = buffer + strlen(buffer);
				while(temp != buffer)
				{
					temp --;

					if(*temp == '/' || *temp == '\\')
					{
						*temp = '\0';
						break;
					}
				}

				return RNSTR(buffer);
#endif
#if RN_PLATFORM_ANDROID
				return RNCSTR("");
#endif

				break;
			}

			case Location::RootResourcesDirectory:
			{
#if RN_PLATFORM_MAC_OS
				if(isAppBundle)
				{
					NSBundle *bundle = [NSBundle mainBundle];
					NSString *path = [bundle resourcePath];

					return RNSTR([path UTF8String]);
				}
				else
				{
					NSArray *arguments = [[NSProcessInfo processInfo] arguments];
					NSString *directory = [arguments objectAtIndex:0];

					String *result = RNSTR([directory UTF8String]);
					result = result->StringByDeletingLastPathComponent();

					return result;
				}
#endif
#if RN_PLATFORM_WINDOWS
				char buffer[MAX_PATH];
				DWORD size = MAX_PATH;

				::GetModuleFileNameA(0, buffer, size);

				char *temp = buffer + strlen(buffer);
				while(temp != buffer)
				{
					temp --;

					if(*temp == '/' || *temp == '\\')
					{
						*temp = '\0';
						break;
					}
				}

				return RNSTR(buffer);
#endif
#if RN_PLATFORM_LINUX
				char buffer[PATH_MAX];
				size_t size = PATH_MAX;
				readlink("/proc/self/exe", buffer, size);

				char *temp = buffer + strlen(buffer);
				while(temp != buffer)
				{
					temp --;

					if(*temp == '/' || *temp == '\\')
					{
						*temp = '\0';
						break;
					}
				}

				return RNSTR(buffer);
#endif
#if RN_PLATFORM_ANDROID
				return RNCSTR("");
#endif

				break;
			}

			case Location::SaveDirectory:
			{
#if RN_PLATFORM_MAC_OS
				const String *application = Kernel::GetSharedInstance()->GetApplication()->GetTitle();

				NSURL *url = [[[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask] lastObject];
				url = [url URLByAppendingPathComponent:[NSString stringWithUTF8String:application->GetUTF8String()]];

	//			[[NSFileManager defaultManager] createDirectoryAtURL:url withIntermediateDirectories:YES attributes:nil error:NULL];

				return RNSTR([[url path] UTF8String]);

#endif
#if RN_PLATFORM_WINDOWS
				const String *application = Kernel::GetSharedInstance()->GetApplication()->GetTitle();

				TCHAR tpath[MAX_PATH];
				::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, tpath);

				String *path = RNSTR(tpath << "/" << application);

//				::CreateDirectory(path->GetUTF8String(), NULL);
				return path;
#endif
#if RN_PLATFORM_LINUX
				char *home = getenv("HOME");
				const String *application = Kernel::GetSharedInstance()->GetApplication()->GetTitle();
				String *path = RNSTR(home << "/." << application);
//				mkdir(path->GetUTF8String(), S_IRWXU);
				return path;
#endif
#if RN_PLATFORM_ANDROID
				const char *dataPath = Kernel::GetSharedInstance()->GetAndroidApp()->activity->internalDataPath;
				return RNSTR(dataPath);
#endif
				break;
			}
		}

		return nullptr;
	}
	
	bool FileManager::RenameFile(const String *oldPath, const String *newPath, bool overwrite)
	{
		if(overwrite) std::remove(newPath->GetUTF8String());
		return std::rename(oldPath->GetUTF8String(), newPath->GetUTF8String()) == 0;
	}
	
	bool FileManager::CreateDirectory(const String *path)
	{
#if RN_PLATFORM_MAC_OS
		NSURL *url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path->GetUTF8String()]];
		NSError *error = nullptr;
		[[NSFileManager defaultManager] createDirectoryAtURL:url withIntermediateDirectories:YES attributes:nil error:&error];
		return error == nullptr;
#endif
#if RN_PLATFORM_WINDOWS
		return ::CreateDirectory(path->GetUTF8String(), NULL);
#endif
#if RN_PLATFORM_LINUX || RN_PLATFORM_ANDROID
		return mkdir(path->GetUTF8String(), S_IRWXU) == 0;
#endif
	}

	void FileManager::AddSearchPath(const String *path)
	{
		path = __ExpandPath(path);

		LockGuard<Lockable> lock(_lock);

		bool hasPath = false;

		_nodes->Enumerate<Directory>([&](Directory *directory, size_t index, bool &stop) {
			if(directory->GetPath()->IsEqual(path))
			{
				hasPath = true;
				stop = true;
			}
		});

		if(!hasPath)
		{
			try
			{
				Directory *directory = new Directory(path);
				_nodes->AddObject(directory);
				directory->Release();
			}
			catch(Exception e)
			{
				throw e;
			}
		}
	}

	void FileManager::RemoveSearchPath(const String *path)
	{
		path = __ExpandPath(path);

		LockGuard<Lockable> lock(_lock);

		size_t index = kRNNotFound;

		_nodes->Enumerate<Directory>([&](Directory *directory, size_t tindex, bool &stop) {
			if(directory->GetPath()->IsEqual(path))
			{
				index = tindex;
				stop = true;
			}
		});

		if(index != kRNNotFound)
		{
			_nodes->RemoveObjectAtIndex(index);
		}
	}

	void FileManager::__AddModuleWithPath(Module *module, const String *path)
	{
		String *prefix = SafeCopy(module->GetName());

		prefix->Insert(RNCSTR(":"), 0);
		prefix->Append(RNCSTR(":"));

		{
			LockGuard<Lockable> lock(_lock);

			Array *nodes = _modulePaths->GetObjectForKey<Array>(prefix);
			if(!nodes)
			{
				nodes = new Array();
				_modulePaths->SetObjectForKey(nodes->Autorelease(), prefix);
			}

			try
			{
				Directory *directory = new Directory(path);
				nodes->AddObject(directory);
				directory->Release();
			}
			catch(Exception e)
			{
				throw e;
			}
		}

		prefix->Release();

	}
	void FileManager::__RemoveModule(Module *module)
	{
		String *prefix = SafeCopy(module->GetName());

		prefix->Insert(RNCSTR(":"), 0);
		prefix->Append(RNCSTR(":"));

		LockGuard<Lockable> lock(_lock);
		_modulePaths->RemoveObjectForKey(prefix);
	}

	bool FileManager::PathExists(const String *path)
	{
		bool ignored;
		return PathExists(path, ignored);
	}

	bool FileManager::PathExists(const String *path, bool &isDirectory)
	{
#if RN_PLATFORM_POSIX
		struct stat buf;
		int result = stat(path->GetUTF8String(), &buf);

		if(result != 0)
			return false;

		isDirectory = S_ISDIR(buf.st_mode);
		return true;
#endif

#if RN_PLATFORM_WINDOWS
		DWORD attributes = ::GetFileAttributes(path->GetUTF8String());

		if(attributes == INVALID_FILE_ATTRIBUTES)
			return false;

		isDirectory = (attributes & FILE_ATTRIBUTE_DIRECTORY);
		return true;
#endif
	}
}
