//
//  RNFileManager.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_FILEMANAGER_H__
#define __RAYNE_FILEMANAGER_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNArray.h"
#include "RNFile.h"

#define kRNFileSystemNodeChangedMessage RNCSTR("kRNFileSystemNodeChangedMessage")

#define kRNFileSystemNodeAddedNodesKey   RNCSTR("kRNFileSystemNodeAddedNodesKey")
#define kRNFileSystemNodeRemovedNodesKey RNCSTR("kRNFileSystemNodeRemovedNodesKey")

namespace RN
{
	class FileManager;
	class DirectoryProxy;
	
	class FileSystemNode : public Object
	{
	public:
		const std::string& GetName() const { return _name; }
		const std::string& GetPath() const { return _path; }
		
		DirectoryProxy *GetParent() const { return _parent; }
		
		virtual bool IsFile() const { return false; }
		virtual bool IsDirectory() const { return false; }
		
	protected:
		FileSystemNode(const std::string& name, DirectoryProxy *parent);
		
		void SetPath(const std::string& path);
		
	private:
		DirectoryProxy *_parent;
		std::string _name;
		std::string _path;
	};
	
	class FileProxy : public FileSystemNode
	{
	public:
		friend class DirectoryProxy;
		
		File *Open(File::FileMode mode = File::FileMode::Read) const;
		
		bool IsFile() const override { return true; }
		
	private:
		FileProxy(const std::string& name, DirectoryProxy *parent);
		
		RNDefineMeta(FileProxy, FileSystemNode)
	};
	
	class DirectoryProxy : public FileSystemNode
	{
	public:
		friend class FileManager;
		
		~DirectoryProxy() override;
		
		FileSystemNode *GetSubNode(const std::string& name) const;
		Array *GetSubNodes() const { return const_cast<Array *>(&_nodes); }
		
		bool IsDirectory() const override { return true; }
		
		void __HandleEvent(void *event);
		
	private:
		DirectoryProxy(const std::string& path, bool watchDirectory);
		DirectoryProxy(const std::string& name, DirectoryProxy *parent);
		
		void ScanDirectory();
		void MergeDirectory();
		
		Array _nodes;
		std::unordered_map<std::string, FileSystemNode *> _nodeMap;
		
#if RN_PLATFORM_MAC_OS
		void *_eventStream;
#endif
		
		RNDefineMeta(DirectoryProxy, FileSystemNode)
	};
	
	
	
	class FileManager : public Singleton<FileManager>
	{
	public:
		FileManager();
		~FileManager();
		
		bool AddSearchPath(const std::string& path);
		void RemoveSearchPath(const std::string& path);
		
		Array *GetSearchPaths() const { Array *result = new Array(_directories); return result->Autorelease(); }
		
	private:
		Array _directories;
	};
}

#endif /* __RAYNE_FILEMANAGER_H__ */
