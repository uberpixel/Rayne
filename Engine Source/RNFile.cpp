//
//  RNFile.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNFile.h"
#include "RNBaseInternal.h"

#define kRNFileBufferSize 256

namespace RN
{
	static std::vector<std::string> FileSearchPaths;
	static std::vector<std::string> FileModifiers;

	File::File(const std::string& path, FileMode mode)
	{
		_file = 0;
		_size = 0;
		_mode = mode;

		if(!OpenPath(path, mode))
			throw ErrorException(0, 0, 0, "Could not open file \"" + path + "\"");

		fseek(_file, 0, SEEK_END);
		_size = ftell(_file);
		fseek(_file, 0, SEEK_SET);
	}

	File::~File()
	{
		if(_file)
			fclose(_file);
	}

	// Reading operations

	std::string File::String()
	{
		long read = 0;
		long offset = ftell(_file);
		fseek(_file, 0, SEEK_SET);

		std::string string = std::string();
		char buffer[kRNFileBufferSize];

		string.reserve(_size);

		while(read < _size)
		{
			long tread = fread(buffer, 1, kRNFileBufferSize, _file);
			read += tread;

			if(tread < kRNFileBufferSize && read < _size)
			{
				throw ErrorException(0, 0, 0);
			}

			string.append(buffer, tread);
		}

		fseek(_file, offset, SEEK_SET);
		return string;
	}

	void File::ReadIntoBuffer(void *buffer, size_t size)
	{
		size_t read = fread(buffer, size, 1, _file);
		RN_ASSERT0(read == 1);
	}

	uint8 File::ReadUint8()
	{
		uint8 result;
		ReadIntoBuffer(&result, sizeof(uint8));

		return result;
	}

	uint16 File::ReadUint16()
	{
		uint16 result;
		ReadIntoBuffer(&result, sizeof(uint16));

		return result;
	}

	uint32 File::ReadUint32()
	{
		uint32 result;
		ReadIntoBuffer(&result, sizeof(uint32));

		return result;
	}

	uint64 File::ReadUint64()
	{
		uint64 result;
		ReadIntoBuffer(&result, sizeof(uint64));

		return result;
	}

	int8 File::ReadInt8()
	{
		int8 result;
		ReadIntoBuffer(&result, sizeof(int8));

		return result;
	}

	int16 File::ReadInt16()
	{
		int16 result;
		ReadIntoBuffer(&result, sizeof(int16));

		return result;
	}

	int32 File::ReadInt32()
	{
		int32 result;
		ReadIntoBuffer(&result, sizeof(int32));

		return result;
	}

	int64 File::ReadInt64()
	{
		int64 result;
		ReadIntoBuffer(&result, sizeof(int64));

		return result;
	}

	float File::ReadFloat()
	{
		float result;
		ReadIntoBuffer(&result, sizeof(float));

		return result;
	}

	double File::ReadDouble()
	{
		double result;
		ReadIntoBuffer(&result, sizeof(double));

		return result;
	}

	// Writing operations

	void File::WriteString(const std::string& string)
	{
		size_t length = string.length();
		const char *cstring = string.c_str();

		while(length > 0)
		{
			size_t written = fwrite(cstring, 1, length, _file);

			length -= written;
			cstring += written;
		}
	}


	bool File::OpenPath(const std::string& path, FileMode mode)
	{
		AddDefaultSearchPaths();

		const char *fmode = 0;
		switch(mode)
		{
			case Read:
				fmode = "rb";
				break;

			case Write:
				fmode = "wb";
				break;

			case ReadWrite:
				fmode = "r+b";
				break;
		}


		bool hasExtension = false;
		bool hasName = false;

		size_t i = path.length();
		size_t length = 0;
		size_t extensionIndex = i;

		while(i > 0)
		{
			length ++;
			i --;

			if(!hasExtension)
			{
				if(path[i] == '.')
				{
					hasExtension = true;
					_extension   = path.substr(i + 1, length - 1);
					extensionIndex = i;

					length = 0;
					i --;
				}
			}
			else
			{
				if(path[i] == '/' || path[i] == '\\')
				{
					_name = path.substr(i + 1, length);
					_path = path.substr(0, i);

					hasName = true;
					break;
				}
			}
		}

		if(!hasName)
			_name = path.substr(0, extensionIndex);

		_fullPath = path;
		_file = fopen(_fullPath.c_str(), fmode);
		if(_file)
			return true;

		std::string basepath = (_path.length() > 0) ? "/" + _path + "/" : "/";

		for(auto i=FileSearchPaths.begin(); i!=FileSearchPaths.end(); i++)
		{
			std::string temp = *i + basepath;

			for(auto j=FileModifiers.begin(); j!=FileModifiers.end(); j++)
			{
				_fullPath = temp + _name + *j + "." + _extension;
				_file = fopen(_fullPath.c_str(), fmode);

				if(_file)
					return true;
			}

			_fullPath = temp + _name + "." + _extension;
			_file = fopen(_fullPath.c_str(), fmode);

			if(_file)
				return true;

		}

		return false;
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
			FileModifiers.push_back("~mac");
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
			char buffer[MAX_PATH];
			DWORD size = MAX_PATH;

			GetModuleFileNameA(0, buffer, size);

			char *temp = buffer + strlen(buffer);
			while(temp != buffer)
			{
				temp --;

				if(*temp == '\\')
				{
					*temp = '\0';
					break;
				}
			}

			AddSearchPath(std::string(buffer));
			AddSearchPath(std::string(buffer) + "/Engine Resources");

#ifndef NDEBUG
			while(temp != buffer)
			{
				temp --;

				if(*temp == '\\')
				{
					*temp = '\0';
					break;
				}
			}

			AddSearchPath(std::string(buffer));
			AddSearchPath(std::string(buffer) + "/Engine Resources");
#endif

			FileModifiers.push_back("~150");
			FileModifiers.push_back("~140");
			FileModifiers.push_back("~130");
			FileModifiers.push_back("~120");
			FileModifiers.push_back("~110");
#endif



#if RN_PLATFORM_LINUX
			char *path = get_current_dir_name();
			puts(path);
			File::AddSearchPath(std::string(path));
			File::AddSearchPath(std::string(path) + "/Engine Resources");
			free(path);

			FileModifiers.push_back("~150");
			FileModifiers.push_back("~140");
			FileModifiers.push_back("~130");
			FileModifiers.push_back("~120");
			FileModifiers.push_back("~110");
			FileModifiers.push_back("~linux");
#endif // RN_PLATFORM_LINUX

			addedDefaultSearchPaths = true;
		}
	}

	std::string File::PathForName(const std::string& path)
	{
		File *temp = new File(path, Read);
		if(temp)
		{
			std::string path = std::string(temp->_fullPath);
			temp->Release();

			return path;
		}

		return "";
	}

	std::string File::ExecutableDirectory()
	{
#if RN_PLATFORM_WINDOWS
		char buffer[MAX_PATH];
		DWORD size = MAX_PATH;

		GetModuleFileNameA(0, buffer, size);

		char *temp = buffer + strlen(buffer);
		while(temp != buffer)
		{
			temp --;

			if(*temp == '\\')
			{
				*temp = '\0';
				break;
			}
		}

		return std::string(buffer);
#endif

		return "";
	}
	
	std::string File::SaveDirectory()
	{
#if RN_PLATFORM_MAC_OS
		NSString *path = [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) objectAtIndex:0];
		path = [path stringByAppendingPathComponent:@"Rayne"];
		
		return std::string([path UTF8String]);
#endif
		
#if RN_PLATFORM_LINUX
		const char *basepath = "~/.Rayne";
		
		wordexp_t result;
		wordexp(basepath, &result, 0);
		
		std::string path = std::string(exp_result.we_wordv[0]);
		
		wordfree(result);
		
		return path;
#endif
	}
}
