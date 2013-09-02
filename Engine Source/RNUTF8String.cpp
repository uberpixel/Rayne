//
//  RNUTF8String.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUTF8String.h"
#include "RNAlgorithm.h"
#include "RNData.h"

#define kMutableUTF8StringIncreaseSize 48

namespace RN
{
	RNDeclareMeta(ConstantUTF8String)
	RNDeclareMeta(UTF8String)
	
	static const char UTF8TrailingBytes[256] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
	};
	
	RN_INLINE bool IsLegalUTF8(const uint8 *sequence)
	{
		size_t length = UTF8TrailingBytes[*sequence] + 1;
		const uint8 *end = sequence + length;
		uint8 character;
		
		switch(length)
		{
			case 4:
				if((character = (*--end)) < 0x80 || character > 0xBF)
					return false;
			case 3:
				if((character = (*--end)) < 0x80 || character > 0xBF)
					return false;
			case 2:
				if((character = (*--end)) > 0xBF)
					return false;
				
				switch(*sequence)
				{
					case 0xe0:
						if(character < 0xa0)
							return false;
						break;
						
					case 0xed:
						if(character > 0x9f)
							return false;
						break;
						
					case 0xf0:
						if(character < 0x90)
							return false;
						break;
						
					case 0xf4:
						if(character > 0x8f)
							return false;
						break;
						
					default:
						if(character < 0x80)
							return false;
						break;
				}
				
			case 1:
				if(*sequence >= 0x80 && *sequence < 0xc2)
					return false;
				break;
				
			default:
				return false;
		}
		
		if(*sequence > 0xf4)
			return false;
		
		return true;
	}
	
	RN_INLINE size_t UTF8ByteLength(const uint8 *bytes)
	{
		const uint8 *begin = bytes;
		
		while(*bytes != '\0')
		{
			bytes += UTF8TrailingBytes[*bytes] + 1;
		}
		
		return bytes - begin;
	}
	
	RN_INLINE UniChar UTF8ToUnicode(const uint8 *bytes)
	{
		size_t length = UTF8TrailingBytes[*bytes] + 1;
		UniChar result;
		
		switch(length)
		{
			case 1:
				result = *bytes;
				break;
				
			case 2:
				result = (*bytes ^ 0xc0);
				break;
				
			case 3:
				result = (*bytes ^ 0xe0);
				break;
				
			case 4:
				result = (*bytes ^ 0xf0);
				break;
				
			default:
				throw Exception(Exception::Type::InconsistencyException, "");
		}
		
		bytes ++;
		
		for(size_t i = length; i > 1; i--)
		{
			result <<= 6;
			result |= (*bytes ^ 0x80);
			
			bytes ++;
		}
		
		return result;
	}
	
	RN_INLINE uint8 *SkipCharacters(uint8 *string, size_t skip)
	{
		for(size_t i=0; i<skip; i++)
			string += (UTF8TrailingBytes[*string] + 1);
		
		return string;
	}
	
	
	ConstantUTF8String::ConstantUTF8String()
	{}
	
	ConstantUTF8String::ConstantUTF8String(const uint8 *bytes) :
		ConstantUTF8String(bytes, UTF8ByteLength(bytes))
	{}
	
	ConstantUTF8String::ConstantUTF8String(const uint8 *string, size_t size) :
		_size(size)
	{
		_string = const_cast<uint8 *>(string);
		_length = 0;
		
		const uint8 *bytes = string;
		
		for(size_t i=0; i<size; i++)
		{
			bytes += (UTF8TrailingBytes[*bytes] + 1);
			_length ++;
		}
	}
	
	BasicString *ConstantUTF8String::SimpleCopy() const
	{
		BasicString *string = const_cast<BasicString *>(static_cast<const BasicString *>(this));
		
		string->Retain();
		return string;
	}
	
	BasicString *ConstantUTF8String::MutableCopy() const
	{
		return new UTF8String(_string, _size);
	}
	
	void *ConstantUTF8String::BytesWithEncoding(Encoding encoding, bool lossy, size_t *length) const
	{
		char *data = nullptr;
		size_t size;
		
		switch(encoding)
		{
			case Encoding::ASCII:
			{
				uint8 *bytes = _string;
				data = new char[_length + 1];
				
				for(size_t i=0; i<_length; i++)
				{
					size_t tsize = UTF8TrailingBytes[*bytes] + 1;
					
					if(tsize > 1)
					{
						if(!lossy)
						{
							delete [] data;
							return nullptr;
						}
						
						data[i] = '?';
						bytes += tsize;
						
						continue;
					}
					
					data[i] = static_cast<char>(*bytes);
					bytes ++;
				}
				
				if(length)
					*length = _length;
				
				data[_length] = '\0';
			}
				
			case Encoding::UTF8:
				size = _size;
				data = new char[_size + 1];
				
				data[_size] = '\0';
				std::copy(_string, _string + _size, data);
				break;
				
			default:
				throw Exception(Exception::Type::GenericException, "");
		}
		
		if(data)
		{
			if(length)
				*length = size;
			
			RN::Data *temp = new RN::Data(data, size, true, true);
			temp->Autorelease();
			
			return temp->GetBytes();
		}
		
		return nullptr;
	}
	
	void *ConstantUTF8String::Data() const
	{
		return _string;
	}
	
	bool ConstantUTF8String::IsMutable() const
	{
		return false;
	}
	
	size_t ConstantUTF8String::Length() const
	{
		return _length;
	}
	
	Encoding ConstantUTF8String::CharacterEncoding() const
	{
		return Encoding::UTF8;
	}
	
	UniChar ConstantUTF8String::CharacterAtIndex(size_t index) const
	{
		if(index > _length)
			throw Exception(Exception::Type::RangeException, "Index must be within string bounds");
		
		const uint8 *bytes = SkipCharacters(_string, index);
		return UTF8ToUnicode(bytes);
	}
	
	void ConstantUTF8String::CharactersInRange(UniChar *buffer, const Range& range) const
	{
		if(range.GetEnd() > _length)
			throw Exception(Exception::Type::RangeException, "Range must be within string bounds");
		
		const uint8 *bytes = SkipCharacters(_string, range.origin);
		
		for(size_t i=0; i<range.length; i++)
		{
			*buffer ++ = UTF8ToUnicode(bytes);
			bytes += (UTF8TrailingBytes[*bytes] + 1);
		}
	}
	
	machine_hash ConstantUTF8String::Hash() const
	{
		size_t length = Length();
		machine_hash hash = 0;
		
		uint8 *bytes = _string;
		
		for(size_t i=0; i<length; i++)
		{
			HashCombine(hash, UTF8ToUnicode(bytes));
			bytes += (UTF8TrailingBytes[*bytes] + 1);
		}
		
		return hash;
	}
	
	BasicString *ConstantUTF8String::Substring(const Range& range) const
	{
		uint8 *bytes = SkipCharacters(_string, range.origin);
		uint8 *end = SkipCharacters(bytes, range.length);
		
		return new UTF8String(bytes, end - bytes);
	}
	
	
	
	UTF8String::UTF8String(const uint8 *string) :
		UTF8String(string, UTF8ByteLength(string))
	{}
	
	UTF8String::UTF8String(const uint8 *bytes, size_t size)
	{
		_string = new uint8[size + 1];
		_string[size] = '\0';
		_length = 0;
		
		_size      = size;
		_allocated = size;
		
		std::copy(bytes, bytes + size, _string);
		
		while(*bytes)
		{
			bytes += (UTF8TrailingBytes[*bytes] + 1);
			_length ++;
		}
	}
	
	UTF8String::~UTF8String()
	{
		delete [] _string;
	}
	
	BasicString *UTF8String::SimpleCopy() const
	{
		return new UTF8String(_string, Size());
	}
	
	void UTF8String::AllocateSpace(size_t size)
	{
		if(size > _allocated)
		{
			size = size + kMutableUTF8StringIncreaseSize;
			
			uint8 *temp = new uint8[size + 1];
			temp[_size] = '\0';
			
			std::copy(_string, _string + _size, temp);
			delete [] _string;
			
			_allocated = size;
			_string    = temp;
		}
	}
	
	void UTF8String::ReplaceCharactersInRange(const Range& range, BasicString *string)
	{
		if(string)
		{
			Encoding encoding = string->CharacterEncoding();
			if(encoding == Encoding::ASCII || encoding == Encoding::UTF8)
			{
				uint8 *data = static_cast<uint8 *>(string->Data());
				size_t size = string->Length();
				
				if(encoding == Encoding::UTF8)
				{
					ConstantUTF8String *tstring = static_cast<ConstantUTF8String *>(string);
					size = tstring->Size();
				}
				
				
				uint8 *gap = SkipCharacters(_string, range.origin);
				uint8 *gapEnd = SkipCharacters(gap, range.length);
				
				size_t replace = gapEnd - gap;
				
				if(replace != size)
				{
					size_t oldSize = _size;
					if(size > replace)
					{
						size_t diff = size - replace;
						size_t offset = gap - _string;
						
						AllocateSpace(_size + diff);
						
						_size += diff;
						_string[_size] = '\0';
						
						gap = _string + offset;
						gapEnd = gap + replace;
					}
					
					uint8 *append = SkipCharacters(gap, string->Length());
					std::copy(gapEnd, _string + oldSize, append);
				}
				
				std::copy(data, data + size, gap);
				_length = (_length - range.length) + string->Length();
			}
		}
		else
		{
			uint8 *temp = SkipCharacters(_string, range.origin);
			uint8 *gap  = SkipCharacters(temp, range.length);
			uint8 *end  = _string + _size;
			
			size_t size = end - temp;
			std::copy(gap, gap + size, temp);
			
			_size   -= size;
			_length -= range.length;
			
			_string[_size] = '\0';
		}
	}
	
	bool UTF8String::IsMutable() const
	{
		return true;
	}
}
