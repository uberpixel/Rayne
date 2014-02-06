//
//  RNFile.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNFile.h"
#include "RNPathManager.h"
#include "RNFileManager.h"
#include "RNBaseInternal.h"

#define kRNFileBufferSize 1024

namespace RN
{
	RNDefineMeta(File)

	File::File(const std::string& path, FileMode mode)
	{
		_file = 0;
		_size = 0;
		_mode = mode;

		if(!OpenPath(path, mode))
			throw Exception(Exception::Type::GenericException, "Could not open file \"" + path + "\"");

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

	std::string File::GetString()
	{
		long read = 0;
		long offset = ftell(_file);
		fseek(_file, 0, SEEK_SET);

		std::string string;
		char buffer[kRNFileBufferSize];

		string.reserve(_size);

		while(read < _size)
		{
			long tread = fread(buffer, 1, kRNFileBufferSize, _file);
			read += tread;

			if(tread < kRNFileBufferSize && read < _size)
				throw Exception(Exception::Type::GenericException, "Failed to read data from file!");

			string.append(buffer, tread);
		}

		fseek(_file, offset, SEEK_SET);
		return string;
	}

	std::vector<uint8> File::GetBytes()
	{
		long read = 0;
		long offset = ftell(_file);
		fseek(_file, 0, SEEK_SET);
		
		std::vector<uint8> bytes;
		bytes.reserve(_size);
		
		uint8 *temp = bytes.data();
		
		while(read < _size)
		{
			long tread = fread(temp, 1, kRNFileBufferSize, _file);
			read += tread;
			temp += tread;
			
			if(tread < kRNFileBufferSize && read < _size)
				throw Exception(Exception::Type::GenericException, "Failed to read data from file!");
		}
		
		fseek(_file, offset, SEEK_SET);
		return bytes;
	}
	
	
	void File::ReadIntoString(std::string& string, size_t size, bool appendNull)
	{
		char *buffer = new char[size + 1];
		fread(buffer, size, 1, _file);
		buffer[size] = '\0';
		
		std::string temp(buffer);
		std::swap(temp, string);
		
		delete[] buffer;
	}
	
	void File::ReadIntoBuffer(void *buffer, size_t size)
	{
		size_t read = fread(buffer, size, 1, _file);
		RN_ASSERT(read == 1, "");
	}
	
	void File::Seek(size_t offset, bool fromStart)
	{
		fseek(_file, offset, fromStart ? SEEK_SET : SEEK_CUR);
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
	void File::WriteString(const std::string& string, bool includeNull)
	{
		size_t length = string.length() + (includeNull ? 1 : 0);
		const char *cstring = string.c_str();

		size_t written = fwrite(cstring, 1, length, _file);
		RN_ASSERT(written == length, "");
	}
	
	void File::WriteBuffer(const void *buffer, size_t size)
	{
		const char *data = static_cast<const char *>(buffer);

		size_t written = fwrite(data, 1, size, _file);
		RN_ASSERT(written == size, "");
	}
	
	void File::WriteUint8(uint8 value)
	{
		WriteBuffer(&value, sizeof(uint8));
	}
	
	void File::WriteUint16(uint16 value)
	{
		WriteBuffer(&value, sizeof(uint16));
	}
	
	void File::WriteUint32(uint32 value)
	{
		WriteBuffer(&value, sizeof(uint32));
	}
	
	void File::WriteUint64(uint64 value)
	{
		WriteBuffer(&value, sizeof(uint64));
	}
	
	void File::WriteInt8(int8 value)
	{
		WriteBuffer(&value, sizeof(int8));
	}
	
	void File::WriteInt16(int16 value)
	{
		WriteBuffer(&value, sizeof(int16));
	}
	
	void File::WriteInt32(int32 value)
	{
		WriteBuffer(&value, sizeof(int32));
	}
	
	void File::WriteInt64(int64 value)
	{
		WriteBuffer(&value, sizeof(int64));
	}
	
	void File::WriteFloat(float value)
	{
		WriteBuffer(&value, sizeof(float));
	}
	
	void File::WriteDouble(double value)
	{
		WriteBuffer(&value, sizeof(double));
	}

	// Misc
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


		try
		{
			_fullPath = FileManager::GetSharedInstance()->GetFilePathWithName(path, !(mode == Write || mode == ReadWrite));
		}
		catch(Exception)
		{
			return false;
		}
		
		_path = PathManager::Basepath(_fullPath);
		_name = PathManager::Basename(_fullPath);
		_extension = PathManager::Extension(_fullPath);
		
		_file = fopen(_fullPath.c_str(), fmode);
		return (_file != 0);
	}
}
