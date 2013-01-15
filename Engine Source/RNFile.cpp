//
//  RNFile.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNFile.h"

#define kRNFileBufferSize 256

namespace RN
{
	static std::vector<std::string> FileSearchPaths;
	static std::vector<std::string> FileModifiers;
	
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
		
		_file = FileForPath(_path, 0);
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
			
			std::copy(buffer, buffer + tread, &data[read]);
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
				if(tread > 0)
					string.append(buffer, tread);

				break;
			}
			
			string.append(buffer, tread);
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
			File::AddSearchPath(std::string([path UTF8String]) + "/Engine Resources");
			
			FileModifiers.push_back("~150");
			FileModifiers.push_back("~140");
			FileModifiers.push_back("~130");
			FileModifiers.push_back("~120");
			FileModifiers.push_back("~110");
#endif
			
#if RN_PLATFORM_IOS
			NSString *path = [[NSBundle mainBundle] resourcePath];
			File::AddSearchPath(std::string([path UTF8String]));
			File::AddSearchPath(std::string([path UTF8String]) + "/Engine Resources");
			
			FileModifiers.push_back("~es2");
			FileModifiers.push_back("~ios");
			
			if(UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
			{
				FileModifiers.push_back("~iphone~es2");
				FileModifiers.push_back("~iphone");
			}
			else if(UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
			{
				FileModifiers.push_back("~ipad~es2");
				FileModifiers.push_back("~ipad");
			}
#endif

#if RN_PLATFORM_WINDOWS
			File::AddSearchPath(std::string("C://Projects/Rayne/Engine Resources"));

			FileModifiers.push_back("~150");
			FileModifiers.push_back("~140");
			FileModifiers.push_back("~130");
			FileModifiers.push_back("~120");
			FileModifiers.push_back("~110");
#endif
			
			addedDefaultSearchPaths = true;
		}
	}
	
	std::string File::PathForName(const std::string& path)
	{
		AddDefaultSearchPaths();
		
		std::string outPath;
		FILE *file = FileForPath(path, &outPath);
		if(file)
		{
			fclose(file);
			return outPath;
		}
		
		return "";
	}
	
	FILE *File::FileForPath(const std::string& fullpath, std::string *outPath)
	{
		AddDefaultSearchPaths();
		
		FILE *file = fopen(fullpath.c_str(), "rb");
		if(file)
		{
			if(outPath)
				*outPath = fullpath;
			
			return file;
		}
		
		std::string path("");
		std::string name("");
		std::string extension("");
		
		bool hasExtension = false;
		bool hasName = false;
		
		size_t i = fullpath.length();
		size_t length = 0;
		size_t extensionIndex = i;
		
		while(i > 0)
		{
			length ++;
			i --;
			
			if(!hasExtension)
			{
				if(fullpath[i] == '.')
				{
					hasExtension = true;
					extension = fullpath.substr(i + 1, length - 1);
					extensionIndex = i;
					
					length = 0;
					i --;
				}
			}
			else
			{
				if(fullpath[i] == '/' || fullpath[i] == '\\')
				{
					name = fullpath.substr(i + 1, length);
					path = fullpath.substr(0, i);
					
					hasName = true;
					
					break;
				}
			}
		}
		
		if(!hasName)
			name = fullpath.substr(0, extensionIndex);

		
		std::string basepath = (path.length() > 0) ? "/" + path + "/" : "/";
		
		for(auto i=FileSearchPaths.begin(); i!=FileSearchPaths.end(); i++)
		{
			std::string temp = *i + basepath;
			
			for(auto j=FileModifiers.begin(); j!=FileModifiers.end(); j++)
			{
				std::string exploded = temp + name + *j + "." + extension;

				file = fopen(exploded.c_str(), "rb");
				if(file)
				{
					if(outPath)
						*outPath = exploded;
					
					return file;
				}
			}
			
			std::string exploded = temp + name + "." + extension;
			
			file = fopen(exploded.c_str(), "rb");
			if(file)
			{
				if(outPath)
					*outPath = exploded;
				
				return file;
			}
			
		}
		
		return 0;
	}
}
