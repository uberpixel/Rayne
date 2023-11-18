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
	#define O_BINARY 0x0
#elif RN_PLATFORM_WINDOWS
	#include "../Base/RNUnistd.h"
#endif

#include <sys/stat.h>

#include "RNData.h"
#include "RNSerialization.h"
#include "RNString.h"
#include "../System/RNFileManager.h"
#include "../System//RNFile.h"

#define kRNDataIncreaseLength 64
#define kRNDataReadBufferSize 1024

namespace RN
{
	RNDefineMeta(Data, Object)
	
	Data::Data()
	{
		_length = 0;
		_allocated = 10;
		_bytes = (uint8 *)malloc(_allocated);
		
		_ownsData = _freeData = true;
	}

	Data::Data(size_t length) :
		Data(nullptr, length, false, true)
	{
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
			free(_bytes);
	}
	
	
	
	Data::Data(Deserializer *deserializer)
	{
		uint8 *data = static_cast<uint8 *>(deserializer->DecodeBytes(&_length));
		
		_allocated = _length;
		_freeData  = _ownsData = true;
		_bytes     = (uint8 *)malloc(_allocated);
		
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

	Expected<Data *> Data::WithContentsOfFile(const String *path)
	{
		File *file = File::WithName(path, File::Mode::Read | File::Mode::NoCreate);
		if(!file)
			return InvalidArgumentException(RNSTR("Couldn't open file " << path));

		size_t size = file->GetSize();
		uint8 *bytes = (uint8 *)malloc(size);
		file->Read(bytes, size);

		Data *data = new Data(bytes, size, false, true);
		return data->Autorelease();
	}

	
	
	void Data::Initialize(const void *bytes, size_t length)
	{
		_ownsData = true;
		_freeData = true;
		
		_allocated = length;
		_length    = length;
		
		_bytes = (uint8 *)malloc(_allocated);
		
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
			uint8 *temp = (uint8 *)malloc(minimumLength + kRNDataIncreaseLength);
			std::copy(_bytes, _bytes + _length, temp);
			free(_bytes);
			
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
	
	void Data::WriteToFile(const String *path)
	{
		File *file = File::WithName(path, File::Mode::Write);
		if(!file)
			throw InvalidArgumentException(RNSTR("Couldn't create file " << path));

		file->Write(_bytes, _length);
	}
	
	
	Data *Data::GetDataInRange(Range range) const
	{
		if(range.origin + range.length > _length)
			throw RangeException("range is not within the datas bounds!");
		
		uint8 *data = (uint8 *)malloc(range.length);
		std::copy(_bytes + range.origin, _bytes + range.origin + range.length, data);
		
		Data *temp = new Data(data, range.length, true, true);
		return temp->Autorelease();
	}
	
	void Data::GetBytesInRange(void *buffer, Range range) const
	{
		if(range.origin + range.length > _length)
			throw RangeException("range is not within the datas bounds!");
		
		uint8 *data = static_cast<uint8 *>(buffer);
		std::copy(_bytes + range.origin, _bytes + range.origin + range.length, data);
	}
}
