//
//  RNFileManager.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#import <Foundation/Foundation.h>
#import <CoreServices/CoreServices.h>
#include <dirent.h>

#include "RNFileManager.h"
#include "RNPathManager.h"
#include "RNMessage.h"

namespace RN
{
	RNDeclareMeta(FileProxy)
	RNDeclareMeta(DirectoryProxy)
	
	// ---------------------
	// MARK: -
	// MARK: FileSystemNode
	// ---------------------
	
	FileSystemNode::FileSystemNode(const std::string& name, DirectoryProxy *parent)
	{
		_name = name;
		_parent = parent;
		
		if(_parent)
		{
			_path = PathManager::Join(_parent->GetPath(), _name);
		}
		else
		{
			_path = "";
		}
	}
	
	void FileSystemNode::SetPath(const std::string& path)
	{
		_path = path;
	}
	
	// ---------------------
	// MARK: -
	// MARK: FileProxy
	// ---------------------
	
	FileProxy::FileProxy(const std::string& name, DirectoryProxy *parent) :
		FileSystemNode(name, parent)
	{}
	
	File *FileProxy::Open(File::FileMode mode) const
	{
		File *file = new File(GetPath(), mode);
		return file->Autorelease();
	}
	
	// ---------------------
	// MARK: -
	// MARK: DirectoryEvent
	// ---------------------
	
	class DirectoryEvent
	{
	public:
		DirectoryEvent()
		{
			rescanSubdirectories = false;
			
			itemCreated = false;
			itemRemoved = false;
			itemRenamed = false;
			itemModified = false;
			
			isFile = false;
			isDirectory = false;
			isSymlink = false;
		}
		
#if RN_PLATFORM_MAC_OS
		DirectoryEvent(FSEventStreamEventFlags flags)
		{
			rescanSubdirectories = (flags & kFSEventStreamEventFlagMustScanSubDirs);
			
			itemCreated  = (flags & kFSEventStreamEventFlagItemCreated);
			itemRemoved  = (flags & kFSEventStreamEventFlagItemRemoved);
			itemRenamed  = (flags & kFSEventStreamEventFlagItemRenamed);
			itemModified = (flags & kFSEventStreamEventFlagItemModified);
			
			isFile      = (flags & kFSEventStreamEventFlagItemIsFile);
			isDirectory = (flags & kFSEventStreamEventFlagItemIsDir);
			isSymlink   = (flags & kFSEventStreamEventFlagItemIsSymlink);
		}
#endif
		
		bool NeedsRescan() const
		{
			return (itemCreated || itemRemoved || itemRenamed || rescanSubdirectories);
		}
		
		bool rescanSubdirectories;
		
		bool itemCreated;
		bool itemRemoved;
		bool itemRenamed;
		bool itemModified;
		
		bool isFile;
		bool isDirectory;
		bool isSymlink;
	};
	
	// ---------------------
	// MARK: -
	// MARK: DirectoryProxy
	// ---------------------
	
	static void RNEventsCallback(ConstFSEventStreamRef streamRef,
								 void *callbackCtxInfo,
								 size_t numEvents,
								 void *eventPaths, // CFArrayRef
								 const FSEventStreamEventFlags eventFlags[],
								 const FSEventStreamEventId eventIds[]);
	
	DirectoryProxy::DirectoryProxy(const std::string& path) :
		FileSystemNode(PathManager::Basename(path), nullptr)
	{
		SetPath(path);
		ScanDirectory();
		
#if RN_PLATFORM_MAC_OS
		
		FSEventStreamContext context;
		context.version			= 0;
		context.info			= static_cast<void *>(this);
		context.retain			= NULL;
		context.release			= NULL;
		context.copyDescription	= NULL;
		
		NSArray *watchedPaths = @[ [NSString stringWithUTF8String:path.c_str()] ];
		
		FSEventStreamCreateFlags streamFlags = (kFSEventStreamCreateFlagUseCFTypes | kFSEventStreamCreateFlagWatchRoot);
		FSEventStreamRef stream = FSEventStreamCreate(kCFAllocatorDefault, &RNEventsCallback, &context, reinterpret_cast<CFArrayRef>(watchedPaths), kFSEventStreamEventIdSinceNow, ((NSTimeInterval)1.5), streamFlags);
	
		FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		if(!FSEventStreamStart(stream))
			throw Exception(Exception::Type::InconsistencyException, "Failed to install fsevents tap!");
		
		_eventStream = stream;
#endif
	}
	
	DirectoryProxy::DirectoryProxy(const std::string& name, DirectoryProxy *parent) :
		FileSystemNode(name, parent)
	{
		ScanDirectory();
		_eventStream = nullptr;
	}
	
	DirectoryProxy::~DirectoryProxy()
	{
#if RN_PLATFORM_MAC_OS
		FSEventStreamRef stream = static_cast<FSEventStreamRef>(_eventStream);
		
		if(stream)
		{
			FSEventStreamStop(stream);
			FSEventStreamInvalidate(stream);
			FSEventStreamRelease(stream);
		}
#endif
	}
	
	FileSystemNode *DirectoryProxy::GetSubNode(const std::string& name) const
	{
		std::vector<std::string> components = PathManager::PathComoponents(name);
		if(components.size() > 0)
		{
			std::string temp = components.front();
			components.erase(components.begin());
			
			auto iterator = _nodeMap.find(temp);
			if(iterator != _nodeMap.end())
			{
				FileSystemNode *proxy = iterator->second;
				if(components.empty())
					return proxy;
				
				if(proxy->IsDirectory())
				{
					DirectoryProxy *directory = static_cast<DirectoryProxy *>(proxy);
					return directory->GetSubNode(PathManager::PathByJoiningComponents(components));
				}
			}
			
			return nullptr;
		}
		
		auto iterator = _nodeMap.find(name);
		return (iterator != _nodeMap.end()) ? iterator->second : nullptr;
	}
	
	void DirectoryProxy::ScanDirectory()
	{
#if RN_PLATFORM_POSIX
		DIR *dir = opendir(GetPath().c_str());
		RN_ASSERT(dir, "Couldn't open directory '%s'!", GetPath().c_str());
		
		struct dirent *ent;
		while((ent = readdir(dir)))
		{
			if(strlen(ent->d_name) > 0 && ent->d_name[0] != '.')
			{
				FileSystemNode *node = nullptr;
				
				switch(ent->d_type)
				{
					case DT_DIR:
						node = new DirectoryProxy(ent->d_name, this);
						break;
						
					case DT_REG:
						node = new FileProxy(ent->d_name, this);
						break;
						
					default:
						break;
				}
				
				if(node)
				{
					_nodes.AddObject(node);
					_nodeMap.insert(std::unordered_map<std::string, FileSystemNode *>::value_type(node->GetName(), node));
					
					node->Release();
				}
			}
		}
		
		closedir(dir);
#endif
	}
	
	void DirectoryProxy::MergeDirectory()
	{
#if RN_PLATFORM_POSIX
		DIR *dir = opendir(GetPath().c_str());
		RN_ASSERT(dir, "Couldn't open directory '%s'!", GetPath().c_str());
		
		std::unordered_map<std::string, FileSystemNode *> _currentMap = _nodeMap;
		
		Array *addedNodes = new Array();
		Array *removedNodes = new Array();
		
		struct dirent *ent;
		while((ent = readdir(dir)))
		{
			if(strlen(ent->d_name) > 0 && ent->d_name[0] != '.')
			{
				FileSystemNode *node = nullptr;
				bool createNode = false;
				
				if(_currentMap.find(ent->d_name) != _currentMap.end())
				{
					_currentMap.erase(ent->d_name);
				}
				else
				{
					createNode = true;
				}
				
				if(createNode)
				{
					switch(ent->d_type)
					{
						case DT_DIR:
							node = new DirectoryProxy(ent->d_name, this);
							break;
							
						case DT_REG:
							node = new FileProxy(ent->d_name, this);
							break;
							
						default:
							break;
					}
				}
				
				if(node)
				{
					_nodes.AddObject(node);
					_nodeMap.insert(std::unordered_map<std::string, FileSystemNode *>::value_type(node->GetName(), node));
					
					node->Release();
					addedNodes->AddObject(node);
				}
			}
		}
		
		closedir(dir);
		
		// Remove all removed nodes
		for(auto i = _currentMap.begin(); i != _currentMap.end(); i ++)
		{
			FileSystemNode *node = i->second;
			removedNodes->AddObject(node);
			
			_nodeMap.erase(i->first);
			_nodes.RemoveObject(node);
		}
		
		Dictionary *changes = new Dictionary();
		changes->SetObjectForKey(addedNodes, kRNFileSystemNodeAddedNodesKey);
		changes->SetObjectForKey(removedNodes, kRNFileSystemNodeRemovedNodesKey);
		
		MessageCenter::GetSharedInstance()->PostMessage(kRNFileSystemNodeChangedMessage, this, changes);
		
		changes->Release();
		addedNodes->Release();
		removedNodes->Release();
#endif
	}
	
	void DirectoryProxy::__HandleEvent(void *tevent)
	{
		DirectoryEvent *event = static_cast<DirectoryEvent *>(tevent);
		
		if(!event->NeedsRescan())
			return;
		
		
		MergeDirectory();
		
		if(event->rescanSubdirectories)
		{
			_nodes.Enumerate<FileSystemNode>([&](FileSystemNode *node, size_t index, bool *stop) {
				if(node->IsDirectory())
				{
					DirectoryProxy *proxy = static_cast<DirectoryProxy *>(node);
					proxy->__HandleEvent(event);
				}
			});
		}
	}
	
#if RN_PLATFORM_MAC_OS
	static void RNEventsCallback(ConstFSEventStreamRef streamRef,
								 void *info,
								 size_t numEvents,
								 void *eventPaths,
								 const FSEventStreamEventFlags eventFlags[],
								 const FSEventStreamEventId eventIds[])
	{
		DirectoryProxy *proxy    = static_cast<DirectoryProxy *>(info);
		NSArray *eventPathsArray = static_cast<NSArray *>(eventPaths);
		
		std::string base = proxy->GetPath();
		
		for(size_t i = 0; i < numEvents; i++)
		{
			FSEventStreamEventFlags flags = eventFlags[i];
			
			NSURL *eventURL		= [NSURL fileURLWithPath:[[eventPathsArray objectAtIndex:i] stringByStandardizingPath]];
			NSString *eventPath	= [eventURL path];
			
			std::string path = [eventPath UTF8String];
			path.erase(0, base.length());
			
			std::vector<std::string> components = PathManager::PathComoponents(path);
			DirectoryProxy *target = proxy;
			
			for(size_t j = 0; j < components.size(); j ++)
			{
				target = static_cast<DirectoryProxy *>(target->GetSubNode(components[i]));
				RN_ASSERT(target, "");
			}
			
			DirectoryEvent event(flags);
			target->__HandleEvent(&event);
		}
	}
#endif
	
	// ---------------------
	// MARK: -
	// MARK: FileManager
	// ---------------------
	
	FileManager::FileManager()
	{
#if RN_PLATFORM_MAC_OS
		NSString *path = [[NSBundle mainBundle] resourcePath];
		AddSearchPath(std::string([path UTF8String]) + "/Engine Resources");
		
		AddFileModifier("~mac");
#endif
		
#if RN_PLATFORM_LINUX || RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS
		AddFileModifier("~150", "vsh");
		AddFileModifier("~140", "vsh");
		AddFileModifier("~130", "vsh");
		AddFileModifier("~120", "vsh");
		AddFileModifier("~110", "vsh");
		
		AddFileModifier("~150", "fsh");
		AddFileModifier("~140", "fsh");
		AddFileModifier("~130", "fsh");
		AddFileModifier("~120", "fsh");
		AddFileModifier("~110", "fsh");
		
		AddFileModifier("~150", "gsh");
		AddFileModifier("~140", "gsh");
		AddFileModifier("~130", "gsh");
		AddFileModifier("~120", "gsh");
		AddFileModifier("~110", "gsh");
#endif
	}
	
	FileManager::~FileManager()
	{
	}
	
	
	
	
	FileProxy *FileManager::GetFileWithName(const std::string& name, bool strict)
	{
		std::string basePath  = PathManager::PathByRemovingExtension(name);
		std::string extension = PathManager::Extension(name);
		
		std::vector<std::string> modifiers;
		
		auto iterator = _fileModifiers.find(extension);
		if(iterator != _fileModifiers.end())
			modifiers.insert(modifiers.end(), iterator->second.begin(), iterator->second.end());
		
		modifiers.insert(modifiers.end(), _globalModifiers.begin(), _globalModifiers.end());
		extension = "." + extension;
		
		FileSystemNode *file = nullptr;
		
		_directories.Enumerate<DirectoryProxy>([&](DirectoryProxy *directory, size_t tindex, bool *stop) {
						
			for(auto j = modifiers.begin(); j != modifiers.end(); j ++)
			{
				std::string filePath = basePath + *j + extension;
				
				if((file = directory->GetSubNode(filePath)))
				{
					*stop = true;
					return;
				}
			}
			
			std::string filePath = basePath + extension;
			
			if((file = directory->GetSubNode(filePath)))
				*stop = true;
		});
		
		if(file && file->IsFile())
			return static_cast<FileProxy *>(file);
		
		if(!strict)
		{
			try
			{
				std::string tpath = PathManager::Basepath(name);
				file = GetFileWithName(tpath);
				
				if(file && file->IsFile())
					return static_cast<FileProxy *>(file);
			}
			catch(Exception)
			{}
		}
		
		throw Exception(Exception::Type::GenericException, "Couldn't resolve path " + name);
	}
	
	std::string FileManager::GetFilePathWithName(const std::string& name, bool strict)
	{
		bool isDirectory;
		bool exists = PathManager::PathExists(PathManager::Basepath(name), &isDirectory);
		
		if(exists && isDirectory)
		{
			std::string basePath  = PathManager::PathByRemovingExtension(name);
			std::string extension = PathManager::Extension(name);
			
			std::vector<std::string> modifiers;
			
			auto iterator = _fileModifiers.find(extension);
			if(iterator != _fileModifiers.end())
				modifiers.insert(modifiers.end(), iterator->second.begin(), iterator->second.end());
			
			modifiers.insert(modifiers.end(), _globalModifiers.begin(), _globalModifiers.end());
			extension = "." + extension;
			
			for(auto j = modifiers.begin(); j != modifiers.end(); j ++)
			{
				std::string filePath = basePath + *j + extension;
				
				if(PathManager::PathExists(filePath))
					return filePath;
			}
			
			
			if(PathManager::PathExists(name))
				return name;
		}
		
		FileProxy *proxy = GetFileWithName(name, strict);
		return proxy->GetPath();
	}
	
	
	
	void FileManager::AddFileModifier(const std::string& modifier, const std::string& extension)
	{
		auto iterator = _fileModifiers.find(extension);
		if(iterator != _fileModifiers.end())
		{
			std::vector<std::string>& modifiers = iterator->second;
			modifiers.push_back(modifier);
		}
		else
		{
			std::vector<std::string> modifiers;
			modifiers.push_back(modifier);
			
			_fileModifiers.insert(std::unordered_map<std::string, std::vector<std::string>>::value_type(extension, modifiers));
		}
	}
	
	void FileManager::AddFileModifier(const std::string& modifier)
	{
		_globalModifiers.push_back(modifier);
	}
	
	
	
	bool FileManager::AddSearchPath(const std::string& path)
	{
		bool hasPath = false;
		
		_directories.Enumerate<DirectoryProxy>([&](DirectoryProxy *directory, size_t index, bool *stop) {
			if(directory->GetPath() == path)
			{
				hasPath = true;
				*stop = true;
			}
		});
		
		if(!hasPath)
		{
			try
			{
				DirectoryProxy *proxy = new DirectoryProxy(path);
				_directories.AddObject(proxy);
				proxy->Release();
				
				MessageCenter::GetSharedInstance()->PostMessage(kRNFileSystemSearchPathsChangedMessage, nullptr, nullptr);
			}
			catch(Exception e)
			{
				return false;
			}
		}
		
		return true;
	}
	
	void FileManager::RemoveSearchPath(const std::string& path)
	{
		size_t index = k::NotFound;
		
		_directories.Enumerate<DirectoryProxy>([&](DirectoryProxy *directory, size_t tindex, bool *stop) {
			if(directory->GetPath() == path)
			{
				index = tindex;
				*stop = true;
			}
		});
		
		if(index != k::NotFound)
		{
			_directories.RemoveObjectAtIndex(index);
			MessageCenter::GetSharedInstance()->PostMessage(kRNFileSystemSearchPathsChangedMessage, nullptr, nullptr);
		}
	}
}
