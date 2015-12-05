//
//  RNData.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//
#include "../Base/RNBase.h"
#include <fcntl.h>
#if RN_PLATFORM_POSIX
	#include <unistd.h>
#elif RN_PLATFORM_WINDOWS
	#include "../Base/RNUnistd.h"
#endif
#include <sys/stat.h>

#include "RNData.h"
#include "RNSerialization.h"
#include "RNString.h"
#include "../System/RNFileCoordinator.h"

#define kRNDataIncreaseLength 64
#define kRNDataReadBufferSize 1024

namespace RN
{
	RNDefineMeta(Data, Object)
	
	Data::Data()
	{
		_length = 0;
		_allocated = 10;
		_bytes = new uint8[_allocated];
		
		_ownsData = _freeData = true;
	}
	
	Data::Data(const void *bytes, size_t length) :
		Data(bytes, length, false, true)
	{
	}
	
	Data::Data(const void *bytes, size_t length, bool noCopy, bool deleteWhenDone)
	{
		if(noCopy)
		{
			_bytes = const_cast<uint8 *>(static_cast<const uint8 *>(bytes));
			_allocated = 0;
			_length    = length;
			
			_ownsData = false;
			_freeData = deleteWhenDone;
		}
		else
		{
			Initialize(bytes, length);
		}
	}

	
	Data::Data(const Data *other)
	{
		Initialize(other->_bytes, other->_length);
	}
	
	Data::~Data()
	{
		if(_freeData)
			delete[] _bytes;
	}
	
	
	
	Data::Data(Deserializer *deserializer)
	{
		uint8 *data = static_cast<uint8 *>(deserializer->DecodeBytes(&_length));
		
		_allocated = _length;
		_freeData  = _ownsData = true;
		_bytes     = new uint8[_allocated];
		
		std::copy(data, data + _length, _bytes);
	}
	
	void Data::Serialize(Serializer *serializer) const
	{
		serializer->EncodeBytes(_bytes, _length);
	}
	
	
	
	Data *Data::WithBytes(const uint8 *bytes, size_t length)
	{
		Data *data = new Data(bytes, length);
		return data->Autorelease();
	}

	Expected<Data *> Data::WithContentsOfFile(const String *file)
	{
		String *path = FileCoordinator::GetSharedInstance()->ResolveFullPath(file, 0);
		if(!path)
			return nullptr;

		int fd = _open(path->GetUTF8String(), O_RDONLY|O_BINARY);
		if(fd < 0)
			return InvalidArgumentException("Couldn't open file");

		off_t size = lseek(fd, 0, SEEK_END);
		lseek(fd, 0, SEEK_SET);

		uint8 *bytes = new uint8[size];
		size_t bytesRead = 0;

		while(bytesRead < size)
		{
			ssize_t result = _read(fd, bytes + bytesRead, size - bytesRead);
			if(result < 0)
			{
				if(errno == EINTR)
					continue;

				_close(fd);
				return InconsistencyException("Failed to read file");
			}
			else if(result == 0)
			{
				break;
			}
			else
				bytesRead += result;
		}

		_close(fd);

		Data *data = new Data(bytes, size, true, true);
		return data->Autorelease();
	}

	
	
	void Data::Initialize(const void *bytes, size_t length)
	{
		_ownsData = true;
		_freeData = true;
		
		_allocated = length;
		_length    = length;
		
		_bytes = new uint8[_allocated];
		
		if(bytes)
		{
			const uint8 *data = static_cast<const uint8 *>(bytes);
			std::copy(data, data + _length, _bytes);
		}
	}
	
	void Data::AssertSize(size_t minimumLength)
	{
		if(!_ownsData)
			throw InconsistencyException("The Data object doesn't own it's data and can't modify it!");
		
		if(_allocated < minimumLength)
		{
			uint8 *temp = new uint8[minimumLength + kRNDataIncreaseLength];
			std::copy(_bytes, _bytes + _length, temp);
			delete [] _bytes;
			
			_bytes = temp;
			_allocated = minimumLength + kRNDataIncreaseLength;
		}
	}
	
	
	void Data::Append(const void *bytes, size_t length)
	{
		AssertSize(_length + length);
		const uint8 *data = static_cast<const uint8 *>(bytes);
		
		std::copy(data, data + length, _bytes + _length);
		_length += length;
	}
	
	void Data::Append(Data *other)
	{
		Append(other->_bytes, other->_length);
	}
	
	void Data::ReplaceBytes(const void *bytes, const Range &range)
	{
		if(range.origin + range.length > _length)
			throw RangeException("range is not within the datas bounds!");
		
		const uint8 *data = static_cast<const uint8 *>(bytes);
		std::copy(data, data + range.length, _bytes + range.origin);
	}
	
	bool Data::WriteToFile(const String *file)
	{
		String *path = FileCoordinator::GetSharedInstance()->ResolveFullPath(file, FileCoordinator::ResolveHint::CreateNode);
		if(!path)
			return false;

		int fd = _open(path->GetUTF8String(), O_WRONLY | O_TRUNC | O_CREAT | O_BINARY);
		if(fd < 0)
			return false;

		_chmod(path->GetUTF8String(), 0755);

		size_t written = 0;
		while(written < _length)
		{
			ssize_t result = _write(fd, _bytes + written, _length - written);
			if(result < 0)
			{
				if(errno == EINTR)
					continue;

				_close(fd);
				return false;
			}

			written += result;
		}

		_close(fd);
		return true;
	}
	
	
	Data *Data::GetDataInRange(Range range) const
	{
		if(range.origin + range.length > _length)
			throw RangeException("range is not within the datas bounds!");
		
		uint8 *data = new uint8[range.length];
		std::copy(_bytes + range.origin, _bytes + range.origin + range.length, data);
		
		Data *temp = new Data(data, range.length, true, true);
		return temp->Autorelease();
	}
	
	void Data::GetBytesInRange(void *buffer, Range range) const
	{
		if(range.origin + range.length > _length)
			throw RangeException("ange is not within the datas bounds!");
		
		uint8 *data = static_cast<uint8 *>(buffer);
		std::copy(_bytes + range.origin, _bytes + range.origin + range.length, data);
	}
}
