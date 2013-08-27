//
//  RNString.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNString.h"
#include "RNFile.h"
#include "RNBasicString.h"

namespace RN
{
	RNDeclareMeta(String)

	// ---------------------
	// MARK: -
	// MARK: String
	// ---------------------
	
#define _ainternal static_cast<BasicString *>(_internal)
	
	String::String()
	{
		_internal = StringFactory::EmptyString()->Retain();
		_encoding = _ainternal->CharacterEncoding();
	}
	
	String::String(const char *string, va_list args)
	{
		va_list temp;
		va_copy(temp, args);
		
		size_t size = vsnprintf(nullptr, 0, string, temp) + 1;
		char *formatted = new char[size + 1];
		
		vsnprintf(formatted, size, string, args);
		va_end(temp);
		
		_internal = StringFactory::ConstructString(formatted, Encoding::ASCII, StringTraits::Mutable);
		_encoding = _ainternal->CharacterEncoding();
		
		delete [] formatted;
	}
	
	String::String(const char *string, bool constant)
	{
		StringTraits traits = constant ? StringTraits::Constant : StringTraits::Mutable;
		
		_internal = StringFactory::ConstructString(string, Encoding::ASCII, traits);
		_encoding = _ainternal->CharacterEncoding();
	}
	
	String::String(const char *string, size_t length, bool constant)
	{
		StringTraits traits = constant ? StringTraits::Constant : StringTraits::Mutable;
		
		_internal = StringFactory::ConstructString(string, length, Encoding::ASCII, traits);
		_encoding = _ainternal->CharacterEncoding();
	}
	
	String::String(const void *bytes, Encoding encoding, bool constant)
	{
		StringTraits traits = constant ? StringTraits::Constant : StringTraits::Mutable;
		
		_internal = StringFactory::ConstructString(bytes, encoding, traits);
		_encoding = _ainternal->CharacterEncoding();
	}
	
	String::String(const void *bytes, size_t length, Encoding encoding, bool constant)
	{
		StringTraits traits = constant ? StringTraits::Constant : StringTraits::Mutable;
		
		_internal = StringFactory::ConstructString(bytes, length, encoding, traits);
		_encoding = _ainternal->CharacterEncoding();
	}
	
	String::String(void *internal)
	{
		_internal = internal;
		_encoding = _ainternal->CharacterEncoding();
	}

	
	
	String::String(const String *string)
	{
		BasicString *other = static_cast<BasicString *>(string->_internal);
		_internal = other->Copy();
	}
	
	String::~String()
	{
		_ainternal->Release();
	}
	
	
	
	String *String::WithFormat(const char *string, ...)
	{
		va_list args;
		va_start(args, string);
		
		String *temp = new String(string, args);
		va_end(args);
		
		return temp->Autorelease();
	}
	
	String *String::WithString(const char *string, bool constant)
	{
		if(constant)
		{
			void *data = const_cast<char *>(string);
			return StringFactory::DequeueConstantString(data, Encoding::ASCII);
		}
		
		String *temp = new String(string, false);
		return temp->Autorelease();
	}
	
	String *String::WithString(const char *string, size_t length, bool constant)
	{
		String *temp = new String(string, length, false);
		return temp->Autorelease();
	}
	
	String *String::WithBytes(const void *bytes, Encoding encoding, bool constant)
	{
		if(constant)
		{
			void *data = const_cast<void *>(bytes);
			return StringFactory::DequeueConstantString(data, encoding);
		}
		
		String *string = new String(bytes, encoding, false);
		return string->Autorelease();
	}
	
	String *String::WithBytes(const void *bytes, size_t length, Encoding encoding, bool constant)
	{
		String *string = new String(bytes, length, encoding, false);
		return string->Autorelease();
	}
	
	String *String::WithContentsOfFile(const std::string& tfile, Encoding encoding)
	{
		File *file = new File(tfile);
		std::vector<uint8> bytes = std::move(file->GetBytes());
		file->Release();
		
		String *string = new String(bytes.data(), encoding, false);
		return string->Autorelease();
	}
	
	// ---------------------
	// MARK: -
	// MARK: Object system
	// ---------------------
	
	machine_hash String::GetHash() const
	{
		return _ainternal->Hash();
	}
	
	bool String::IsEqual(Object *other) const
	{
		if(!other->IsKindOfClass(String::MetaClass()))
		   return false;
		   
		String *string = static_cast<String *>(other);
		if(string->GetLength() != GetLength())
			return false;
		
		if(_internal == string->_internal)
			return true;
		
		return (Compare(string) == ComparisonResult::EqualTo);
	}
	
	// ---------------------
	// MARK: -
	// MARK: Mutation
	// ---------------------
	
	void String::PromoteStringIfNeeded(Encoding encoding)
	{
		if(!_ainternal->IsMutable())
		{
			BasicString *copy = _ainternal->MutableCopy();
			
			_ainternal->Release();
			_internal = copy;
		}
		
		bool needsReEncoding = (_encoding == Encoding::ASCII && encoding != Encoding::ASCII);
		if(needsReEncoding)
		{
			BasicString *copy = StringFactory::ConstructString(_ainternal, encoding);
			
			_ainternal->Release();
			_internal = copy;
		}
	}
	
	void String::Append(const char *string, ...)
	{
		va_list args;
		va_start(args, string);
		
		String *temp = new String(string, args);
		Append(temp);
		temp->Release();
		
		va_end(args);
	}
	
	void String::Append(const String *string)
	{
		BasicString *other = static_cast<BasicString *>(string->_internal);
		
		PromoteStringIfNeeded(other->CharacterEncoding());
		_ainternal->ReplaceCharactersInRange(Range(GetLength(), 0), other);
	}
	
	void String::Insert(const String *string, size_t index)
	{
		PromoteStringIfNeeded(_ainternal->CharacterEncoding());
		ReplaceCharacters(string, Range(index, 0));
	}
	
	void String::DeleteCharacters(const Range& range)
	{
		PromoteStringIfNeeded(_ainternal->CharacterEncoding());
		_ainternal->ReplaceCharactersInRange(range, nullptr);
	}
	
	void String::ReplaceCharacters(const String *replacement, const Range& range)
	{
		BasicString *other = nullptr;
		
		if(replacement)
		{
			other = static_cast<BasicString *>(replacement->_internal);
			PromoteStringIfNeeded(other->CharacterEncoding());
		}
		
		_ainternal->ReplaceCharactersInRange(range, other);
	}
	
	void String::ReplaceOccurrencesOfString(const String *string, const String *replacement)
	{
		while(1)
		{
			Range range = GetRangeOfString(string);
			if(range.origin == kRNNotFound)
				break;
			
			ReplaceCharacters(replacement, range);
		}
	}
	
	// ---------------------
	// MARK: -
	// MARK: Comparison
	// ---------------------
	
#define kStringUniCharFetch 128
	
	Range String::GetRangeOfString(const String *string, ComparisonMode mode)
	{		
		return GetRangeOfString(string, mode, Range(0, GetLength()));
	}
	
	Range String::GetRangeOfString(const String *string, ComparisonMode mode, const Range& range)
	{
		size_t length = string->GetLength();
		
		if(range.length < length)
			return Range(kRNNotFound, 0);
		
		UniChar characters[kStringUniCharFetch];
		UniChar *compare = new UniChar[length];
		
		BasicString *other = static_cast<BasicString *>(string->_internal);
		other->CharactersInRange(compare, Range(0, length));
		
		bool found = false;
		size_t left   = range.length;
		size_t offset = range.origin;
		size_t j = 1;
		
		Range result = Range(kRNNotFound, 0);
		CodePoint first = CodePoint(compare[0]);
		
		while(left > 0 && !found)
		{
			size_t read = std::min(left, static_cast<size_t>(kStringUniCharFetch));
			_ainternal->CharactersInRange(characters, Range(offset, read));
			
			for(size_t i = 0; i < read; i ++)
			{
				CodePoint point = characters[i];
				
				if(result.origin == kRNNotFound)
				{
					if(point == first)
					{
						result.origin = offset + i;
						result.length = 1;
						
						if(result.length >= length)
						{
							found = true;
							break;
						}
					}
				}
				else
				{
					CodePoint tpoint = CodePoint(compare[j]);
					
					if(mode & ComparisonModeCaseInsensitive)
					{
						point  = point.GetLowerCase();
						tpoint = tpoint.GetLowerCase();
					}
					
					if(tpoint != point)
					{
						j = 1;
						result = Range(kRNNotFound, 0);
					}
					else
					{
						result.length ++;
						j ++;
						
						if(result.length >= length)
						{
							found = true;
							break;
						}
					}
				}
			}
			
			offset += read;
			left   -= read;
		}
		
		delete [] compare;
		return result;
	}
	
	ComparisonResult String::Compare(const String *other, ComparisonMode mode) const
	{
		return Compare(other, mode, Range(0, GetLength()));
	}
	
	ComparisonResult String::Compare(const String *other, ComparisonMode mode, const Range& range) const
	{
		if(range.length < other->GetLength())
			return ComparisonResult::GreaterThan;
		
		if(range.length > other->GetLength())
			return ComparisonResult::LessThan;
		
		UniChar charactersA[kStringUniCharFetch];
		UniChar charactersB[kStringUniCharFetch];
		
		BasicString *internalA = static_cast<BasicString *>(_internal);
		BasicString *internalB = static_cast<BasicString *>(other->_internal);
		
		size_t iA = 0;
		size_t iB = 0;
		
		size_t offsetA = range.origin;
		size_t offsetB = 0;
		
		size_t readA = 0;
		size_t readB = 0;
		
		size_t leftA = range.length;
		size_t leftB = range.length;
		
#define ReadCharacter(Side) \
		do { \
			if(offset##Side + i##Side >= read##Side) \
			{ \
				if(left##Side == 0) \
					goto endComparison; \
					\
				read##Side = std::min(left##Side, static_cast<size_t>(kStringUniCharFetch)); \
				offset##Side += i##Side; \
				internal##Side->CharactersInRange(characters##Side, Range(offset##Side, read##Side)); \
				\
				left##Side -= read##Side; \
				i##Side = 0; \
			} \
		} while(0)
		
		while(1)
		{
			ReadCharacter(A);
			ReadCharacter(B);
			
			CodePoint a = charactersA[iA ++];
			CodePoint b = charactersB[iB ++];
			
			if(mode & ComparisonModeCaseInsensitive)
			{
				a = a.GetLowerCase();
				b = b.GetLowerCase();
			}
			
			if(mode & ComparisonModeNumerically)
			{
				if(a <= 0x7f && b <= 0x7f)
				{
					char ca = static_cast<char>(a);
					char cb = static_cast<char>(b);
					
					if(ca <= '9' && ca >= '0' && cb <= '9' && cb >= '0')
					{
						uint32 numA = 0;
						uint32 numB = 0;
						
						do {
							numA = numA * 10 + (ca - '0');
							
							ReadCharacter(A);
							a = charactersA[iA ++];
							
							ca = static_cast<char>(a);
						} while(ca <= '9' && ca >= '0');
						
						do {
							numB = numB * 10 + (cb - '0');
							
							ReadCharacter(B);
							b = charactersB[iB ++];
							
							cb = static_cast<char>(b);
						} while(cb <= '9' && cb >= '0');
						
						if(numA > numB)
							return ComparisonResult::GreaterThan;
						
						if(numB > numA)
							return ComparisonResult::LessThan;
						
						continue;
					}
				}
			}
			
			if(a != b)
			{
				if(a > b)
					return ComparisonResult::GreaterThan;
				
				if(b > a)
					return ComparisonResult::LessThan;
			}
		}
		
#undef ReadCharacter
		
	endComparison:
	
		if(leftA && !leftB)
			return ComparisonResult::GreaterThan;
		
		if(leftB && !leftA)
			return ComparisonResult::LessThan;
	
		return ComparisonResult::EqualTo;
	}
	
	// ---------------------
	// MARK: -
	// MARK: Conversion / Access
	// ---------------------
	
	size_t String::GetLength() const
	{
		return _ainternal->Length();
	}
	
	char *String::GetUTF8String() const
	{
		return static_cast<char *>(_ainternal->BytesWithEncoding(Encoding::UTF8, false, nullptr));
	}
	
	String *String::GetSubstring(const Range& range) const
	{
		void *string = static_cast<void *>(_ainternal->Substring(range));
		String *substring = new String(string);
		
		return substring->Autorelease();
	}
	
	UniChar String::GetCharacterAtIndex(size_t index) const
	{
		return _ainternal->CharacterAtIndex(index);
	}
	
	uint8 *String::GetBytesWithEncoding(Encoding encoding, bool lossy, size_t *outLength) const
	{		
		return static_cast<uint8 *>(_ainternal->BytesWithEncoding(encoding, lossy, outLength));
	}
}
