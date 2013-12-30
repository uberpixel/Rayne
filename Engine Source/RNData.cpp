//
//  RNData.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNData.h"
#include "RNFile.h"

#define kRNDataIncreaseLength 64

namespace RN
{
	RNDeclareMeta(Data)
	
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
	
	Data::Data(const std::string& name)
	{
		File *file = new File(name, File::FileMode::Read);
		size_t length = file->GetSize();
		
		Initialize(nullptr, length);
		file->ReadIntoBuffer(_bytes, length);
		file->Release();
	}
	
	Data::Data(const Data& other)
	{
		Initialize(other._bytes, other._length);
	}
	
	Data::Data(Data *other)
	{
		Initialize(other->_bytes, other->_length);
	}
	
	Data::~Data()
	{
		if(_freeData)
			delete[] _bytes;
	}
	
	
	Data *Data::WithBytes(const uint8 *bytes, size_t length)
	{
		Data *data = new Data(bytes, length);
		return data->Autorelease();
	}
	
	Data *Data::WithContentsOfFile(const std::string& file)
	{
		Data *data = new Data(file);
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
			throw Exception(Exception::Type::InconsistencyException, "The Data object doesn't own it's data and can't modify it!");
		
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
	
	void Data::ReplaceBytes(const void *bytes, const Range& range)
	{
		if(range.origin + range.length >= _length)
			throw Exception(Exception::Type::RangeException, "range is not within the datas bounds!");
		
		const uint8 *data = static_cast<const uint8 *>(bytes);
		std::copy(data, data + range.length, _bytes + range.origin);
	}
	
	void Data::WriteToFile(const std::string& name)
	{
		File *file = new File(name, File::FileMode::Write);
		file->WriteBuffer(_bytes, _length);
		file->Release();
	}
	
	
	Data *Data::GetDataInRange(Range range) const
	{
		if(range.origin + range.length > _length)
			throw Exception(Exception::Type::RangeException, "range is not within the datas bounds!");
		
		uint8 *data = new uint8[range.length];
		std::copy(_bytes + range.origin, _bytes + range.origin + range.length, data);
		
		Data *temp = new Data(data, range.length, true, true);
		return temp->Autorelease();
	}
	
	void Data::GetBytesInRange(void *buffer, Range range) const
	{
		if(range.origin + range.length > _length)
			throw Exception(Exception::Type::RangeException, "range is not within the datas bounds!");
		
		uint8 *data = static_cast<uint8 *>(buffer);
		std::copy(_bytes + range.origin, _bytes + range.origin + range.length, data);
	}
}
