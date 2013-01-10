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
		File(const std::string& path);
		virtual ~File();
		
		static void AddSearchPath(const std::string& path);
		static std::string PathForName(const std::string& name);
		
		bool IsOpen() const { return (_file != 0); }
		
		void Open();
		void Close();
		
		std::vector<uint8> Bytes();
		std::string String();
		
		FILE *FilePointer() { return _file; }
		
	private:
		static void AddDefaultSearchPaths();
		static FILE *FileForPath(const std::string& path, std::string *outPath);
		
		std::string _path;
		FILE *_file;
		long _size;
	};
}

#endif /* __RAYNE_FILE_H__ */
