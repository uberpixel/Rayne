//
//  RNFileCoordinator.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Base/RNBaseInternal.h"
#include "../Base/RNKernel.h"
#include "../Base/RNApplication.h"
#include "../Modules/RNModule.h"
#include "RNFileCoordinator.h"

#if RN_PLATFORM_POSIX
	#include <dirent.h>
	#include <unistd.h>
	#include <sys/stat.h>
#endif

namespace RN
{
	RNDefineScopedMeta(FileCoordinator, Node, Object)
	RNDefineScopedMeta(FileCoordinator, Directory, FileCoordinator::Node)
	RNDefineScopedMeta(FileCoordinator, File, FileCoordinator::Node)

	FileCoordinator::Node::Node(String *name, Node *parent, Type type) :
		_type(type),
		_name(name->Retain()),
		_path(nullptr),
		_parent(parent)
	{
		if(_parent)
			SetPath(_parent->GetPath()->StringByAppendingPathComponent(name));
	}
	FileCoordinator::Node::~Node()
	{
		SafeRelease(_path);
		_name->Release();
	}

	void FileCoordinator::Node::SetPath(String *path)
	{
		SafeRelease(_path);
		_path = SafeRetain(path);
	}


	FileCoordinator::Directory::Directory(String *name, Node *parent) :
		Node(name, parent, Type::Directory),
		_children(new Array()),
		_childMap(new Dictionary())
	{
		ParseDirectory();
	}

	FileCoordinator::Directory::Directory(const String *path) :
		Node(path->GetLastPathComponent(), nullptr, Type::Directory),
		_children(new Array()),
		_childMap(new Dictionary())
	{
		SetPath(path->Copy()->Autorelease());
		ParseDirectory();
	}

	FileCoordinator::Directory::~Directory()
	{
		_children->Release();
		_childMap->Release();
	}

	FileCoordinator::Node *FileCoordinator::Directory::GetChildWithName(const String *name) const
	{
		return _childMap->GetObjectForKey<FileCoordinator::Node>(const_cast<String *>(name));
	}

	void FileCoordinator::Directory::ParseDirectory()
	{
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
					_children->AddObject(node);
					_childMap->SetObjectForKey(node, const_cast<String *>(node->GetName()));

					node->Release();

				}
			}
		}

		closedir(dir);
	}

	FileCoordinator::File::File(String *name, Node *parent) :
		Node(name, parent, Type::File)
	{}

	// ---------------------
	// MARK: -
	// MARK: FileCoordinator
	// ---------------------

#if RN_PLATFORM_POSIX
	char *realpath_expand(const char *path, char *buffer)
	{
#if RN_PLATFORM_LINUX
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

	static FileCoordinator *__sharedInstance = nullptr;

	FileCoordinator::FileCoordinator() :
		_nodes(new Array()),
		_modulePaths(new Dictionary())
	{
		__sharedInstance = this;
	}
	FileCoordinator::~FileCoordinator()
	{
		SafeRelease(_nodes);
		SafeRelease(_modulePaths);

		__sharedInstance = nullptr;
	}
	FileCoordinator *FileCoordinator::GetSharedInstance()
	{
		return __sharedInstance;
	}

	void FileCoordinator::__PrepareWithManifest()
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

	String *FileCoordinator::__ExpandPath(const String *tpath)
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

		DWORD result = ::GetFullPathNameA(tpath.c_str(), MAX_PATH, buffer, nullptr);
		std::string path((result != 0) ? buffer : tpath.c_str());
#endif

		return path;
	}

	Array *FileCoordinator::__GetNodeContainerForPath(const String *path, Array *&outPath)
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

	FileCoordinator::Node *FileCoordinator::ResolvePath(const String *path, ResolveHint hint) RN_NOEXCEPT
	{
		std::lock_guard<std::mutex> lock(_lock);

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

	String *FileCoordinator::ResolveFullPath(const String *path, ResolveHint hint) RN_NOEXCEPT
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

			return result->GetPath()->Copy()->Autorelease();
		}
		
		String *expanded = __ExpandPath(path);
		
		if(access(expanded->GetUTF8String(), F_OK) != -1)
			return expanded;

		if(hint & ResolveHint::CreateNode)
		{
			String *parent = expanded->StringByDeletingLastPathComponent();
			
			if(access(parent->GetUTF8String(), F_OK) != -1)
				return expanded;
		}

		return nullptr;
	}

	String *FileCoordinator::GetPathForLocation(Location location) const
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

				break;
			}

			case Location::SaveDirectory:
			{
#if RN_PLATFORM_MAC_OS
				const String *application = Kernel::GetSharedInstance()->GetApplication()->GetTitle();

				NSURL *url = [[[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask] lastObject];
				url = [url URLByAppendingPathComponent:[NSString stringWithUTF8String:application->GetUTF8String()]];

				[[NSFileManager defaultManager] createDirectoryAtURL:url withIntermediateDirectories:YES attributes:nil error:NULL];

				return RNSTR([[url path] UTF8String]);

#endif
				break;
			}
		}

		return nullptr;
	}

	void FileCoordinator::AddSearchPath(const String *path)
	{
		path = __ExpandPath(path);

		std::lock_guard<std::mutex> lock(_lock);

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

	void FileCoordinator::RemoveSearchPath(const String *path)
	{
		path = __ExpandPath(path);

		std::lock_guard<std::mutex> lock(_lock);

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

	void FileCoordinator::__AddModuleWithPath(Module *module, const String *path)
	{
		String *prefix = SafeCopy(module->GetName());

		prefix->Insert(RNCSTR(":"), 0);
		prefix->Append(RNCSTR(":"));

		{
			std::lock_guard<std::mutex> lock(_lock);

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
	void FileCoordinator::__RemoveModule(Module *module)
	{
		String *prefix = SafeCopy(module->GetName());

		prefix->Insert(RNCSTR(":"), 0);
		prefix->Append(RNCSTR(":"));

		std::lock_guard<std::mutex> lock(_lock);
		_modulePaths->RemoveObjectForKey(prefix);
	}

	bool FileCoordinator::PathExists(const String *path)
	{
		__unused bool ignored;
		return PathExists(path, ignored);
	}

	bool FileCoordinator::PathExists(const String *path, bool &isDirectory)
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
