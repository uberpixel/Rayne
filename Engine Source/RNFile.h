//
//  RNFile.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_FILE_H__
#define __RAYNE_FILE_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class File : public Object
	{
	public:
		typedef enum
		{
			Read = 0,
			Write = 1,
			ReadWrite = 2
		} FileMode;

		RNAPI File(const std::string& path, FileMode mode = Read);
		RNAPI virtual ~File();
		
		// Reading operations
		RNAPI std::string String();

		// Writing operations
		RNAPI void WriteString(const std::string& string);
		
		// Misc
		FILE *FilePointer() { return _file; }

		RNAPI const std::string& Name() { return _name; }
		RNAPI const std::string& Extension() { return _extension; }
		RNAPI const std::string& Path() { return _path; }
		
		static void AddSearchPath(const std::string& path);
		static std::string PathForName(const std::string& name);
		static std::string ExecutableDirectory();

	private:
		static void AddDefaultSearchPaths();
		
		bool OpenPath(const std::string& path, FileMode mode);

		FileMode _mode;

		std::string _name;
		std::string _extension;
		std::string _path;
		std::string _fullPath;

		FILE *_file;
		long _size;
	};
}

#endif /* __RAYNE_FILE_H__ */
