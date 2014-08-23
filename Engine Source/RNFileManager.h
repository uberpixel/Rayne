//
//  RNFileManager.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_FILEMANAGER_H__
#define __RAYNE_FILEMANAGER_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNArray.h"
#include "RNFile.h"
#include "RNAdaptiveLock.h"

#define kRNFileSystemSearchPathsChangedMessage RNCSTR("kRNFileSystemSearchPathsChangedMessage")
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
		const std::string &GetName() const { return _name; }
		const std::string &GetPath() const { return _path; }
		
		DirectoryProxy *GetParent() const { return _parent; }
		
		virtual bool IsFile() const { return false; }
		virtual bool IsDirectory() const { return false; }
		
	protected:
		RNAPI FileSystemNode(const std::string &name, DirectoryProxy *parent);
		
		RNAPI void SetPath(const std::string &path);
		
	private:
		DirectoryProxy *_parent;
		std::string _name;
		std::string _path;
		
		RNDeclareMeta(FileSystemNode)
	};
	
	class FileProxy : public FileSystemNode
	{
	public:
		friend class DirectoryProxy;
		friend class FileManager;
		
		RNAPI File *Open(File::FileMode mode = File::FileMode::Read) const;
		
		bool IsFile() const override { return true; }
		
	private:
		RNAPI FileProxy(const std::string &name, DirectoryProxy *parent);
		
		RNDeclareMeta(FileProxy)
	};
	
	class DirectoryProxy : public FileSystemNode
	{
	public:
		friend class FileManager;
		
		RNAPI ~DirectoryProxy() override;
		
		RNAPI FileSystemNode *GetSubNode(const std::string &name) const;
		Array *GetSubNodes() const { return const_cast<Array *>(&_nodes); }
		RNAPI Array *GetSubNodesMatchingPredicate(const std::function<bool (FileProxy *file)>& predicate, bool allowFolders = true) const;
		
		bool IsDirectory() const override { return true; }
		
		RNAPI void __HandleEvent(void *event);
		
	private:
		DirectoryProxy(const std::string &path);
		DirectoryProxy(const std::string &name, DirectoryProxy *parent);
		
		void ScanDirectory();
		void MergeDirectory();
		
		Array _nodes;
		std::unordered_map<std::string, FileSystemNode *> _nodeMap;
		
#if RN_PLATFORM_MAC_OS
		void *_eventStream;
#endif
		
		RNDeclareMeta(DirectoryProxy)
	};
	
	
	struct FileManagerInternals;
	
	class FileManager : public ISingleton<FileManager>
	{
	public:
		RNAPI FileManager();
		RNAPI ~FileManager();
		
		RNAPI FileSystemNode *GetFileSystemNode(const std::string &name);
		RNAPI FileProxy *GetFileWithName(const std::string &name, bool strict = true);
		
		RNAPI std::string GetFilePathWithName(const std::string &name, bool strict = true);
		RNAPI std::string GetNormalizedPathFromFullpath(const std::string &name);
		
		RNAPI bool AddSearchPath(const std::string &path);
		RNAPI void RemoveSearchPath(const std::string &path);
		
		RNAPI void AddFileModifier(const std::string &modifier, const std::string &extension);
		RNAPI void AddFileModifier(const std::string &modifier);
		
		RNAPI Array *GetSearchPaths() const;
		
	private:
		PIMPL<FileManagerInternals> _internals;
		
		RNDeclareSingleton(FileManager)
	};
}

#endif /* __RAYNE_FILEMANAGER_H__ */
