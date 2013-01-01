//
//  RNFile.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNFile.h"

#define kRNFileBufferSize 256

namespace RN
{
	std::vector<std::string> FileSearchPaths;	
	
	File::File(const std::string& path)
	{
		_path = path;
		_file = 0;
	}
	
	File::~File()
	{
		if(_file)
			fclose(_file);
	}
	
	
	void File::Open()
	{
		if(_file)
			return;
		
		_file = FileForPath(_path);
		if(_file)
		{
			fseek(_file, 0, SEEK_END);
			_size = ftell(_file);
			
			fseek(_file, 0, SEEK_SET);
		}
	}
	
	void File::Close()
	{
		if(_file)
		{
			fclose(_file);
			_file = 0;
		}
	}
	
	
	std::vector<uint8> File::Bytes()
	{
		bool wasOpen = IsOpen();
		Open();
		
		long read = 0;
		long offset = ftell(_file);
		fseek(_file, 0, SEEK_SET);
		
		std::vector<uint8> data = std::vector<uint8>((size_t)_size);
		uint8 buffer[kRNFileBufferSize];
	
		while(read < _size)
		{
			long tread = fread(buffer, 1, kRNFileBufferSize, _file);
			if(tread < kRNFileBufferSize && read + tread < _size)
			{
				// TODO: Throw an exception!
			}
			
			memcpy(&data[read], buffer, tread);
			read += tread;
		}
		
		if(wasOpen)
		{
			fseek(_file, offset, SEEK_SET);
		}
		else
		{
			Close();
		}
		
		return data;
	}
	
	std::string File::String()
	{
		bool wasOpen = IsOpen();
		Open();
		
		long read = 0;
		long offset = ftell(_file);
		fseek(_file, 0, SEEK_SET);
		
		std::string string = std::string();
		char buffer[kRNFileBufferSize];
		
		string.reserve(_size);
		
		while(read < _size)
		{
			long tread = fread(buffer, 1, kRNFileBufferSize, _file);
			if(tread < kRNFileBufferSize && read + tread < _size)
			{
				// TODO: Throw an exception!
			}
			
			string.append(buffer);
			read += tread;
		}
		
		if(wasOpen)
		{
			fseek(_file, offset, SEEK_SET);
		}
		else
		{
			Close();
		}
		
		return string;
	}
	
	
	
	
	void File::AddSearchPath(const std::string& path)
	{
		FileSearchPaths.push_back(path);
	}
	
	void File::AddDefaultSearchPaths()
	{
		static bool addedDefaultSearchPaths = false;
		if(!addedDefaultSearchPaths)
		{
#if RN_PLATFORM_MAC_OS
			NSString *path = [[NSBundle mainBundle] resourcePath];
			File::AddSearchPath(std::string([path UTF8String]));
#endif
			
			addedDefaultSearchPaths = true;
		}
	}
	
	std::string File::PathForName(const std::string& path)
	{
		AddDefaultSearchPaths();
		
		FILE *file = fopen(path.c_str(), "r");
		if(file)
		{
			fclose(file);
			return path;
		}
		
		for(auto i=FileSearchPaths.begin(); i!=FileSearchPaths.end(); i++)
		{
			std::string tpath = *i + "/" + path;
			
			file = fopen(tpath.c_str(), "r");
			if(file)
			{
				fclose(file);
				return tpath;
			}
		}
		
		return "";
	}
	
	FILE *File::FileForPath(const std::string& path)
	{
		AddDefaultSearchPaths();
		
		FILE *file = fopen(path.c_str(), "r");
		if(file)
			return file;
		
		for(auto i=FileSearchPaths.begin(); i!=FileSearchPaths.end(); i++)
		{
			std::string tpath = *i + "/" + path;
			
			file = fopen(tpath.c_str(), "r");
			if(file)
				return file;
		}
		
		return 0;
	}
}
