//
//  RNFile.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNFile.h"
#include "RNPathManager.h"
#include "RNBaseInternal.h"

#define kRNFileBufferSize 256

namespace RN
{
	static std::vector<std::string> FileSearchPaths;
	static std::vector<std::string> FileModifiers;
	
	RNDeclareMeta(File)

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
	
	void File::Seek(size_t offset)
	{
		fseek(_file, offset, SEEK_CUR);
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


		_fullPath = PathManager::PathForName(path);
		if(_fullPath.size() == 0)
			return false;
		
		_name = PathManager::Basename(_fullPath);
		_extension = PathManager::Extension(_fullPath);
		_path = PathManager::Basepath(_fullPath);
		
		_file = fopen(_fullPath.c_str(), fmode);
		return (_file != 0);
	}
}
