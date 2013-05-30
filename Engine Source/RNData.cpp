//
//  RNData.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
	
	Data::Data(const uint8 *bytes, size_t length) :
		Data(bytes, length, false, true)
	{
	}
	
	Data::Data(const uint8 *bytes, size_t length, bool noCopy, bool deleteWhenDone)
	{
		if(noCopy)
		{
			_bytes = const_cast<uint8 *>(bytes);
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
		size_t length = file->Size();
		
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
	
	
	void Data::Initialize(const uint8 *bytes, size_t length)
	{
		_ownsData = true;
		_freeData = true;
		
		_allocated = length;
		_length    = length;
		
		_bytes = new uint8[_allocated];
		
		if(bytes)
			std::copy(bytes, bytes + _length, _bytes);
	}
	
	void Data::AssertSize(size_t minimumLength)
	{
		if(!_ownsData)
			throw ErrorException(0);
		
		if(_allocated < minimumLength)
		{
			uint8 *temp = new uint8[minimumLength + kRNDataIncreaseLength];
			std::copy(_bytes, _bytes + _length, temp);
			delete [] _bytes;
			
			_bytes = temp;
			_allocated = minimumLength + kRNDataIncreaseLength;
		}
	}
	
	
	void Data::Append(const uint8 *bytes, size_t length)
	{
		AssertSize(_length + length);
		std::copy(bytes + _length, bytes + _length + length, _bytes);
		
		_length += length;
	}
	
	void Data::Append(Data *other)
	{
		Append(other->_bytes, other->_length);
	}
	
	
	void Data::WriteToFile(const std::string& name)
	{
		File *file = new File(name, File::FileMode::Write);
		file->WriteBuffer(_bytes, _length);
		file->Release();
	}
	
	void Data::BytesInRange(uint8 *buffer, Range range) const
	{
		if(range.origin + range.length >= _length)
			throw ErrorException(0);
		
		std::copy(buffer, buffer + range.length, _bytes + range.origin);
	}
}
