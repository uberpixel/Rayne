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

		protected:
			Node(String *name, Node *parent, Type type);
			~Node();

			void SetPath(String *path);

		private:
			Type _type;

			String *_name;
			String *_path;
			Node *_parent;

			RNDeclareMeta(Node)
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

			RNDeclareMeta(Directory)
		};

		class File : public Node
		{
		public:
			friend class Directory;

		protected:
			File(String *name, Node *parent);

			RNDeclareMeta(File)
		};

		RN_OPTIONS(ResolveHint, uint32,
				   CreateNode = (1 << 0),
		           IgnoreModifiers = (1 << 1));

		RNAPI static FileManager *GetSharedInstance();

		RNAPI Node *ResolvePath(const String *path, ResolveHint hint);
		RNAPI String *ResolveFullPath(const String *path, ResolveHint hint);

		RNAPI void AddSearchPath(const String *path);
		RNAPI void RemoveSearchPath(const String *path);

	private:
		FileManager();
		~FileManager();

		String *__ExpandPath(const String *path);

		std::mutex _lock;
		Array *_nodes;
	};
}

#endif /* __RAYNE_FILEMANAGER_H__ */
