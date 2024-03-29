//
//  RNFileManager.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_FILEMANAGER_H__
#define __RAYNE_FILEMANAGER_H__

#include "../Base/RNBase.h"
#include "../Objects/RNString.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNDictionary.h"

namespace RN
{
	class Kernel;
	class FileManager
	{
	public:
		friend class Kernel;
		friend class Module;
		friend class Dictionary;

		class Node : public Object
		{
		public:
			enum class Type
			{
				File,
				Directory
			};

			friend class FileManager;

			Type GetType() const { return _type; }
			Node *GetParent() const { return _parent; }

			const String *GetName() const { return _name; }
			const String *GetPath() const { return _path; }
			const String *GetModifier() const { return _modifier; }

		protected:
			Node(String *name, Node *parent, Type type);
			~Node();

			void SetPath(String *path);

		private:
			Type _type;

			String *_name;
			String *_path;
			String *_modifier;
			Node *_parent;

			__RNDeclareMetaInternal(Node)
		};

		class Directory : public Node
		{
		public:
			friend class FileManager;

			RNAPI Node *GetChildWithName(const String *name) const;
			const Array *GetChildren() const { return _children; }

		protected:
			Directory(String *name, Node *parent);
			Directory(const String *path);
			~Directory();

		private:
			void ParseDirectory();

			Array *_children;
			Dictionary *_childMap;

			__RNDeclareMetaInternal(Directory)
		};

		class File : public Node
		{
		public:
			friend class Directory;

		protected:
			File(String *name, Node *parent);

			__RNDeclareMetaInternal(File)
		};

		RN_OPTIONS(ResolveHint, uint32,
				   CreateNode = (1 << 0),
		           IgnoreModifiers = (1 << 1));

		enum class Location
		{
			ApplicationDirectory,
			RootResourcesDirectory,
			InternalSaveDirectory,
			ExternalSaveDirectory
		};

		RNAPI static FileManager *GetSharedInstance();

		RNAPI Node *ResolvePath(const String *path, ResolveHint hint) RN_NOEXCEPT;
		RNAPI String *ResolveFullPath(const String *path, ResolveHint hint) RN_NOEXCEPT;

		RNAPI String *GetPathForLocation(Location location);
		RNAPI String *GetNormalizedPathFromFullPath(const String *fullPath);
		
		RNAPI bool RenameFile(const String *oldPath, const String *newPath, bool overwrite = true);
		RNAPI bool CreateDirectory(const String *path);
		RNAPI bool DeleteFile(const String *path);
		
		RNAPI Directory *WalkableDirectory(const String *path);

		RNAPI void AddSearchPath(const String *path);
		RNAPI void RemoveSearchPath(const String *path);

		RNAPI static bool PathExists(const String *path);
		RNAPI static bool PathExists(const String *path, bool &isDirectory);

	private:
		FileManager();
		~FileManager();

		String *__ExpandPath(const String *path);
		void __PrepareWithManifest();

		void __AddModuleWithPath(Module *module, const String *path);
		void __RemoveModule(Module *module);

		Array *__GetNodeContainerForPath(const String *path, Array *&outPath);
		Array *GetFilePathsFromZipFile(const String *path) const;

		Lockable _lock;
		Array *_nodes;
		Dictionary *_modulePaths;

		const String *_applicationDirectory;

#if RN_PLATFORM_ANDROID
		Array *_androidAppBundleFiles;
#endif
	};
}

#endif /* __RAYNE_FILEMANAGER_H__ */
