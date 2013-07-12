//
//  RNBasicString.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBasicString.h"
#include "RNASCIIString.h"
#include "RNUTF8String.h"
#include "RNAlgorithm.h"

namespace RN
{
	RNDeclareMeta(BasicString)
	
	BasicString::~BasicString()
	{}
	
	BasicString *BasicString::Copy() const
	{
		throw Exception(Exception::Type::GenericException, "");
	}
	
	UniChar BasicString::CharacterAtIndex(size_t index) const
	{
		throw Exception(Exception::Type::GenericException, "");
	}
	
	void BasicString::CharactersInRange(UniChar *buffer, const Range& range) const
	{
		for(size_t i=0; i<range.length; i++)
		{
			buffer[i] = CharacterAtIndex(i + range.origin);
		}
	}
	
	void BasicString::ReplaceCharactersInRange(const Range& range, BasicString *string)
	{
		throw Exception(Exception::Type::GenericException, "");
	}
	
	
	machine_hash BasicString::Hash() const
	{
		size_t length = Length();
		machine_hash hash = 0;
		
		for(size_t i=0; i<length; i++)
		{
			HashCombine(hash, CharacterAtIndex(i));
		}
		
		return hash;
	}
	
	size_t BasicString::Length() const
	{
		throw Exception(Exception::Type::GenericException, "");
	}
	
	void *BasicString::Data() const
	{
		throw Exception(Exception::Type::GenericException, "");
	}
	
	void *BasicString::BytesWithEncoding(Encoding encoding, bool lossy, size_t *lengt) const
	{
		throw Exception(Exception::Type::GenericException, "");
	}
	
	bool BasicString::IsMutable() const
	{
		throw Exception(Exception::Type::GenericException, "");
	}
	
	Encoding BasicString::CharacterEncoding() const
	{
		throw Exception(Exception::Type::GenericException, "");
	}
	
	BasicString *BasicString::Substring(const Range& range) const
	{
		throw Exception(Exception::Type::GenericException, "");
	}
	
	
	
	BasicString *StringFactory::EmptyString()
	{
		static BasicString *string;
		
		std::once_flag flag;
		std::call_once(flag, [] {
			string = new ConstantASCIIString("");
		});
		
		return string;
	}
	
	BasicString *StringFactory::ConstructString(const void *data, Encoding encoding, StringTraits traits)
	{
		switch(encoding)
		{
			case Encoding::ASCII:
			{
				char *string = static_cast<char *>(const_cast<void *>(data));
				
				switch(traits)
				{
					case StringTraits::Constant:
						return new ConstantASCIIString(string);
						break;
						
					case StringTraits::Mutable:
						return new ASCIIString(string);
						break;
				}
				
				break;
			}
				
			case Encoding::UTF8:
			{
				uint8 *string = static_cast<uint8 *>(const_cast<void *>(data));
				
				switch(traits)
				{
					case StringTraits::Constant:
						return new ConstantUTF8String(string);
						break;
						
					case StringTraits::Mutable:
						return new UTF8String(string);
						break;
				}
				
				break;
			}
				
			default:
				throw Exception(Exception::Type::GenericException, "");
				break;
		}
		
		return nullptr;
	}
	
	BasicString *StringFactory::ConstructString(const void *data, size_t length, Encoding encoding, StringTraits traits)
	{
		switch(encoding)
		{
			case Encoding::ASCII:
			{
				char *string = static_cast<char *>(const_cast<void *>(data));
				return new ASCIIString(string, length);
				break;
			}
				
			case Encoding::UTF8:
			{
				uint8 *string = static_cast<uint8 *>(const_cast<void *>(data));
				return new UTF8String(string, length);
				break;
			}
				
			default:
				throw Exception(Exception::Type::GenericException, "");
				break;
		}
		
		return nullptr;
	}
	
	BasicString *StringFactory::ConstructString(BasicString *other, Encoding encoding)
	{
		switch(encoding)
		{
			case Encoding::UTF8:
			{
				uint8 *string = static_cast<uint8 *>(const_cast<void *>(other->Data()));
				return new UTF8String(string);
				break;
			}
				
			default:
				throw Exception(Exception::Type::GenericException, "");
				break;
		}
		
		return nullptr;
	}
}