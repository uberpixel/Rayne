//
//  RNString.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNString.h"
#include "RNStringInternal.h"
#include "RNSerialization.h"
#include "RNAutoreleasePool.h"

namespace RN
{
	RNDefineMeta(String, Object)

	// ---------------------
	// MARK: -
	// MARK: String
	// ---------------------
	
	String::String() :
		_string(new UTF8String())
	{}
	
	String::String(UTF8String *string) :
		_string(string->Retain())
	{}
	
	String::String(const char *string, va_list args)
	{
		va_list temp;
		va_copy(temp, args);
		
		size_t size = vsnprintf(nullptr, 0, string, temp) + 1;
		char *formatted = new char[size + 1];
		
		vsnprintf(formatted, size, string, args);
		va_end(temp);
		
		_string = new UTF8String(reinterpret_cast<uint8 *>(formatted), strlen(formatted), true);
		
		delete [] formatted;
	}
	
	String::String(const char *string, bool constant)
	{
		if(constant)
		{
			_string = StringPool::CreateUTF8String(string);
			return;
		}

		_string = new UTF8String(reinterpret_cast<const uint8 *>(string), kRNNotFound, !constant);
	}
	
	String::String(const char *string, size_t length, bool constant)
	{
		if(constant)
		{
			_string = StringPool::CreateUTF8String(string);
			return;
		}

		_string = new UTF8String(reinterpret_cast<const uint8 *>(string), length, !constant);
	}
	
	
	String::String(const void *bytes, Encoding encoding, bool constant)
	{
		_string = new UTF8String(bytes, kRNNotFound, encoding, !constant);
	}
	
	String::String(const void *bytes, size_t length, Encoding encoding, bool constant)
	{
		_string = new UTF8String(bytes, length, encoding, !constant);
	}
	
	
	String::String(const String *string)
	{
		_string = string->_string->Copy();
	}
	
	String::~String()
	{
		_string->Release();
	}
	
	
	
	String::String(Deserializer *deserializer)
	{
		size_t length;
		uint8 *bytes = static_cast<uint8 *>(deserializer->DecodeBytes(&length));
		
		_string = new UTF8String(bytes, length, true);
	}
	
	void String::Serialize(Serializer *serializer)
	{
		size_t length;
		uint8 *bytes = GetBytesWithEncoding(Encoding::UTF8, false, length);
		
		serializer->EncodeBytes(bytes, length);
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
		String *temp = new String(string, constant);
		return temp->Autorelease();
	}
	
	String *String::WithString(const char *string, size_t length, bool constant)
	{
		String *temp = new String(string, length, constant);
		return temp->Autorelease();
	}
	
	String *String::WithBytes(const void *bytes, Encoding encoding, bool constant)
	{
		String *string = new String(bytes, encoding, constant);
		return string->Autorelease();
	}
	
	String *String::WithBytes(const void *bytes, size_t length, Encoding encoding, bool constant)
	{
		String *string = new String(bytes, length, encoding, false);
		return string->Autorelease();
	}
	
	String *String::WithContentsOfFile(const std::string &tfile, Encoding encoding)
	{
		/*File *file = new File(tfile);
		std::vector<uint8> bytes = std::move(file->GetBytes());
		size_t size = file->GetSize();
		file->Release();
		
		String *string = new String(bytes.data(), size, encoding, false);
		return string->Autorelease();*/
		return nullptr;
	}
	
	// ---------------------
	// MARK: -
	// MARK: Object system
	// ---------------------
	
	size_t String::GetHash() const
	{
		return _string->GetHash();
	}
	
	bool String::IsEqual(const Object *other) const
	{
		const String *string = other->Downcast<String>();
		if(!string)
			return false;

		if(string->GetLength() != GetLength())
			return false;
		
		return (Compare(string) == ComparisonResult::EqualTo);
	}
	
	// ---------------------
	// MARK: -
	// MARK: Mutation
	// ---------------------
	
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
		_string->ReplaceCharactersInRange(Range(GetLength(), 0), string->_string);
	}
	
	void String::Insert(const String *string, size_t index)
	{
		ReplaceCharacters(string, Range(index, 0));
	}
	
	void String::Capitalize()
	{
		UniChar buffer[128];
		
		size_t read = 0;
		size_t length = _string->GetLength();
		
		bool needsUppercase = true;
		
		while(read < length)
		{
			size_t left = std::min(length - read, static_cast<size_t>(128));
			_string->GetCharactersInRange(buffer, Range(read, left));
			
			for(size_t i = 0; i < left; i ++)
			{
				CodePoint point(buffer[i]);
				CodePoint uppercase = point.GetUpperCase();
				
				if(point.IsWhitespace())
				{
					needsUppercase = true;
					continue;
				}
				
				if(needsUppercase && point != uppercase)
				{
					UniChar temp[2];
					temp[0] = uppercase;
					temp[1] = 0;
					
					UTF8String *string = new UTF8String(temp, 4, Encoding::UTF32, true);
					_string->ReplaceCharactersInRange(Range(read + i, 1), string);
					string->Release();
				}
				
				needsUppercase = false;
			}
			
			read += left;
		}
	}
	
	void String::DeleteCharacters(const Range &range)
	{
		_string->ReplaceCharactersInRange(range, nullptr);
	}
	
	void String::ReplaceCharacters(const String *replacement, const Range &range)
	{
		_string->ReplaceCharactersInRange(range, replacement ? replacement->_string : nullptr);
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
	
	Range String::GetRangeOfString(const String *string, ComparisonMode mode) const
	{		
		return GetRangeOfString(string, mode, Range(0, GetLength()));
	}
	
	Range String::GetRangeOfString(const String *string, ComparisonMode mode, const Range &range) const
	{
		size_t length = string->GetLength();
		
		if(range.length < length)
			return Range(kRNNotFound, 0);
		
		UniChar characters[kStringUniCharFetch];
		UniChar *compare = new UniChar[length];
		
		string->_string->GetCharactersInRange(compare, Range(0, length));
		
		bool found = false;
		size_t left   = range.length;
		size_t offset = range.origin;
		size_t j = 1;
		
		Range result = Range(kRNNotFound, 0);
		CodePoint first = CodePoint(compare[0]);
		
		while(left > 0 && !found)
		{
			size_t read = std::min(left, static_cast<size_t>(kStringUniCharFetch));
			_string->GetCharactersInRange(characters, Range(offset, read));
			
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
					
					if(mode & ComparisonMode::CaseInsensitive)
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
		return found ? result : Range(kRNNotFound, 0);
	}
	
	ComparisonResult String::Compare(const String *other, ComparisonMode mode) const
	{
		return Compare(other, mode, Range(0, GetLength()));
	}
	
	ComparisonResult String::Compare(const String *other, ComparisonMode mode, const Range &range) const
	{
		if(range.length < other->GetLength())
			return ComparisonResult::GreaterThan;
		
		if(range.length > other->GetLength())
			return ComparisonResult::LessThan;
		
		UniChar charactersA[kStringUniCharFetch];
		UniChar charactersB[kStringUniCharFetch];
		
		UTF8String *internalA = _string;
		UTF8String *internalB = other->_string;
		
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
			if(i##Side >= read##Side) \
			{ \
				if(left##Side == 0) \
					goto endComparison; \
					\
				read##Side = std::min(left##Side, static_cast<size_t>(kStringUniCharFetch)); \
				offset##Side += i##Side; \
				internal##Side->GetCharactersInRange(characters##Side, Range(offset##Side, read##Side)); \
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
			
			if(mode & ComparisonMode::CaseInsensitive)
			{
				a = a.GetLowerCase();
				b = b.GetLowerCase();
			}
			
			if(mode & ComparisonMode::Numerically)
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
		return _string->GetLength();
	}
	
	char *String::GetUTF8String() const
	{
		size_t length;
		return static_cast<char *>(_string->GetBytesWithEncoding(Encoding::UTF8, false, length));
	}
	
	String *String::GetSubstring(const Range &range) const
	{
		UTF8String *string = _string->GetSubstring(range);
		String *substring = new String(string);
		
		return substring->Autorelease();
	}
	
	UniChar String::GetCharacterAtIndex(size_t index) const
	{
		return _string->GetCharacterAtIndex(index);
	}
	
	uint8 *String::GetBytesWithEncoding(Encoding encoding, bool lossy, size_t &outLength) const
	{		
		return static_cast<uint8 *>(_string->GetBytesWithEncoding(encoding, lossy, outLength));
	}
	
	Array *String::GetComponentsSeparatedByString(const String *other) const
	{
		Array *array = new Array();
		
		Range range = Range(0, GetLength());
		Range found;
		
		while(1)
		{
			found = GetRangeOfString(other, 0, range);
			
			if(found.origin == kRNNotFound)
			{
				array->AddObject(GetSubstring(range));
				break;
			}
			
			size_t length = found.origin - range.origin;
			array->AddObject(GetSubstring(Range(range.origin, length)));
			
			length += found.length;
			
			if(range.length <= length)
				break;
			
			range.origin += length;
			range.length -= length;
		}
		
		return array->Autorelease();
	}
	
	
	bool String::WriteToFile(const std::string &path, Encoding encoding)
	{
		AutoreleasePool pool;
		
		try
		{
			/*size_t length;
			void *buffer = GetBytesWithEncoding(encoding, true, length);
			
			File *file = new File(path, File::FileMode::Write);
			file->WriteBuffer(buffer, length);
			file->Release();*/
			
			
			return true;
		}
		catch(...)
		{}
		
		return false;
	}
}
