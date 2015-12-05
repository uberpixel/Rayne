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
#include "RNData.h"

#if RN_PLATFORM_POSIX
	#define kRNIsPathDelimiter(c) (c == '/')
	#define kRNPathDelimiter '/'
	#define kRNPathDelimiterTokens "/"
#endif

#if RN_PLATFORM_WINDOWS
	#define kRNIsPathDelimiter(c) (c == '/' || c == '\\')
	#define kRNPathDelimiter '\\'
	#define kRNPathDelimiterTokens "/\\"
#endif

namespace RN
{
	RNDefineMeta(String, Object)

	static CharacterSet *__stringPathDelimiterSet;
	static CharacterSet *__stringPathExtensionSet;

	// ---------------------
	// MARK: -
	// MARK: String
	// ---------------------

	void String::InitialWakeUp(MetaClass *cls)
	{
		if(cls == GetMetaClass())
		{
			__stringPathDelimiterSet = new CharacterSet(kRNPathDelimiterTokens);
			__stringPathExtensionSet = new CharacterSet(".");
		}

		Object::InitialWakeUp(cls);
	}
	
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

	String::String(const Data *data, Encoding encoding) :
		String(data->GetBytes(), data->GetLength(), encoding, false)
	{}
	
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
	
	void String::Serialize(Serializer *serializer) const
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
	
	Expected<String *> String::WithContentsOfFile(const String *file, Encoding encoding)
	{
		Expected<Data *> data = Data::WithContentsOfFile(file);
		if(!data.IsValid())
			return InvalidArgumentException(RNSTR("Couldn't open file '" << file << "'"));

		String *string = new String(data, encoding);
		return string->Autorelease();
	}
	
	// ---------------------
	// MARK: -
	// MARK: Object system
	// ---------------------

	const String *String::GetDescription() const
	{
		return this;
	}
	
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

	String *String::StringByAppendingString(const String *other) const
	{
		String *result = Copy();
		result->Append(other);

		return result->Autorelease();
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
		
		if(range.length < length || range.length == 0)
			return Range(kRNNotFound, 0);
		
		UniChar characters[kStringUniCharFetch];
		UniChar *compare = (length < 256) ? ((UniChar *)alloca(length * sizeof(UniChar))) : new UniChar[length];
		
		string->_string->GetCharactersInRange(compare, Range(0, length));
		
		bool found = false;
		Range result = Range(kRNNotFound, 0);

		if(mode & ComparisonMode::Reverse)
		{
			size_t left   = range.length;
			size_t offset = range.origin + left;
			size_t j = length - 2;

			CodePoint first = CodePoint(compare[length - 1]);

			while(left > 0 && !found)
			{
				size_t read = std::min(left, static_cast<size_t>(kStringUniCharFetch));
				_string->GetCharactersInRange(characters, Range(offset - read, read));

				for(size_t i = 0; i < read; i ++)
				{
					CodePoint point = characters[read - (i + 1)];

					if(result.origin == kRNNotFound)
					{
						if(mode & ComparisonMode::CaseInsensitive)
						{
							point = point.GetLowerCase();
							first = first.GetLowerCase();
						}

						if(point == first)
						{
							result.origin = offset - length - i;
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
							j = length - 2;
							result = Range(kRNNotFound, 0);
						}
						else
						{
							result.length ++;
							j --;

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
		}
		else
		{
			size_t left = range.length;
			size_t offset = range.origin;
			size_t j = 1;

			CodePoint first = CodePoint(compare[0]);

			while(left > 0 && ! found)
			{
				size_t read = std::min(left, static_cast<size_t>(kStringUniCharFetch));
				_string->GetCharactersInRange(characters, Range(offset, read));

				for(size_t i = 0; i < read; i ++)
				{
					CodePoint point = characters[i];

					if(result.origin == kRNNotFound)
					{
						if(mode & ComparisonMode::CaseInsensitive)
						{
							point = point.GetLowerCase();
							first = first.GetLowerCase();
						}

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
							point = point.GetLowerCase();
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
				left -= read;
			}
		}

		if(length >= 256)
			delete [] compare;

		return found ? result : Range(kRNNotFound, 0);
	}

	Range String::GetRangeOfCharacterInSet(const CharacterSet *set, ComparisonMode mode) const
	{
		return GetRangeOfCharacterInSet(set, mode, Range(0, GetLength()));
	}

	Range String::GetRangeOfCharacterInSet(const CharacterSet *set, ComparisonMode mode, const Range &range) const
	{
		UniChar characters[kStringUniCharFetch];

		if(mode & ComparisonMode::Reverse)
		{
			size_t left = range.length;
			size_t offset = range.origin + left;

			while(left > 0)
			{
				size_t read = std::min(left, static_cast<size_t>(kStringUniCharFetch));
				_string->GetCharactersInRange(characters, Range(offset - read, read));

				for(size_t i = 0; i < read; i ++)
				{
					if(set->CharacterIsMember(characters[read - (i + 1)]))
						return Range(offset - (i + 1), 1);
				}

				left -= read;
			}
		}
		else
		{
			size_t left = range.length;
			size_t offset = range.origin;

			while(left > 0)
			{
				size_t read = std::min(left, static_cast<size_t>(kStringUniCharFetch));
				_string->GetCharactersInRange(characters, Range(offset, read));

				for(size_t i = 0; i < read; i ++)
				{
					if(set->CharacterIsMember(characters[i]))
						return Range(offset + i, 1);
				}

				left -= read;
			}
		}

		return Range(kRNNotFound, 0);
	}


	ComparisonResult String::Compare(const String *other, ComparisonMode mode) const
	{
		return Compare(other, mode, Range(0, GetLength()));
	}
	
	ComparisonResult String::Compare(const String *other, ComparisonMode mode, const Range &range) const
	{
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
		size_t leftB = other->GetLength();
		
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

#define CharactersLeft(Side, result) \
		do { \
			if(i##Side >= read##Side && left##Side == 0) \
			{ result = 0; } \
			else \
			{ result = read##Side - i##Side; } \
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
				if(a <= CodePoint::ASCIITerminator() && b <= CodePoint::ASCIITerminator())
				{
					char ca = static_cast<char>(a);
					char cb = static_cast<char>(b);
					
					if(isdigit(ca) && isdigit(cb))
					{
						uint32 numA = 0;
						uint32 numB = 0;
						
						do {
							numA = numA * 10 + (ca - '0');

							size_t left;
							CharactersLeft(A, left);

							if(left == 0)
								break;

							ReadCharacter(A);

							if((a = charactersA[iA ++]) > CodePoint::ASCIITerminator())
								break;
							
							ca = static_cast<char>(a);
						} while(isdigit(ca));
						
						do {
							numB = numB * 10 + (cb - '0');

							size_t left;
							CharactersLeft(B, left);

							if(left == 0)
								break;

							ReadCharacter(B);
							if((b = charactersB[iB ++]) > CodePoint::ASCIITerminator())
								break;

							cb = static_cast<char>(b);
						} while(isdigit(cb));
						
						if(numA > numB)
							return ComparisonResult::GreaterThan;
						
						if(numA < numB)
							return ComparisonResult::LessThan;
						
						continue;
					}
				}
			}
			
			if(a != b)
			{
				if(a > b)
					return ComparisonResult::GreaterThan;
				if(a < b)
					return ComparisonResult::LessThan;
			}
		}
		
#undef ReadCharacter
		
	endComparison:

		size_t _leftA;
		size_t _leftB;
		CharactersLeft(A, _leftA);
		CharactersLeft(B, _leftB);

		if(_leftA && !_leftB)
			return ComparisonResult::GreaterThan;
		if(_leftB && !_leftA)
			return ComparisonResult::LessThan;
	
		return ComparisonResult::EqualTo;
	}

	bool String::HasPrefix(const String *other) const
	{
		if(GetLength() < other->GetLength())
			return false;

		Range range = GetRangeOfString(other, 0, Range(0, other->GetLength()));
		return (range.origin == 0);
	}

	bool String::HasSuffix(const String *other) const
	{
		if(GetLength() < other->GetLength())
			return false;

		size_t offset = GetLength() - other->GetLength();
		Range range = GetRangeOfString(other, 0, Range(offset, other->GetLength()));
		return (range.origin == offset);
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
		return static_cast<char *>(_string->GetBytesWithEncoding(Encoding::UTF8, false, length)->GetBytes());
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
		return static_cast<uint8 *>(_string->GetBytesWithEncoding(encoding, lossy, outLength)->GetBytes());
	}

	Data *String::GetDataWithEncoding(Encoding encoding) const
	{
		size_t length;
		return _string->GetBytesWithEncoding(encoding, true, length);
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

	size_t String::__GetTrailingPathLocation() const
	{
		size_t location = GetLength() - 1;
		size_t result = kRNNotFound;

		while(1)
		{
			Range range(location, 1);
			size_t delimiter = GetRangeOfCharacterInSet(__stringPathDelimiterSet, 0, range).origin;

			if(location == delimiter)
			{
				result = location;

				if(RN_EXPECT_FALSE(location == 0))
					break;

				location --;
				continue;
			}

			break;
		}

		return result;
	}
	void String::__DeleteTrailingPath()
	{
		size_t trailing = __GetTrailingPathLocation();
		if(trailing != kRNNotFound)
			DeleteCharacters(Range(trailing, GetLength() - trailing));
	}

	void String::DeletePathExtension()
	{
		__DeleteTrailingPath();

		// Find the extension and last path delimiter
		size_t extension = GetRangeOfCharacterInSet(__stringPathExtensionSet, ComparisonMode::Reverse).origin;
		if(extension == kRNNotFound)
			return;

		size_t delimiter = GetRangeOfCharacterInSet(__stringPathDelimiterSet, ComparisonMode::Reverse).origin;

		if(delimiter == kRNNotFound || delimiter < extension)
			DeleteCharacters(Range(extension, GetLength() - extension));
	}
	void String::DeleteLastPathComponent()
	{
		if(GetLength() == 0)
			return;

		__DeleteTrailingPath();

		size_t delimiter = GetRangeOfCharacterInSet(__stringPathDelimiterSet, ComparisonMode::Reverse).origin;
		if(delimiter != kRNNotFound)
			DeleteCharacters(Range(delimiter, GetLength() - delimiter));

		if(GetLength() == 0)
			Append("%c", kRNPathDelimiter);
	}
	void String::AppendPathComponent(const String *component)
	{
		if(component->GetCharacterAtIndex(0) != kRNPathDelimiter)
		{
			Range range(GetLength() - 1, 1);
			size_t delimiter = GetRangeOfCharacterInSet(__stringPathDelimiterSet, 0, range).origin;

			if(delimiter == kRNNotFound)
				Append("%c", kRNPathDelimiter);
		}

		Append(component);
	}
	void String::AppendPathExtension(const String *extension)
	{
		__DeleteTrailingPath();

		if(extension->GetCharacterAtIndex(0) != '.')
			Append(".");

		Append(extension);
	}

	Array *String::GetPathComponents() const
	{
		Array *array = new Array();
		Range range(0, GetLength());

		while(kRNIsPathDelimiter(GetCharacterAtIndex(range.origin)))
		{
			range.origin ++;
			range.length --;
		}

		while(1)
		{
			if(range.length == 0)
				break;

			size_t end = GetRangeOfCharacterInSet(__stringPathDelimiterSet, 0, range).origin;

			if(end == range.origin)
			{
				range.origin ++;
				range.length --;

				continue;
			}

			if(end == kRNNotFound)
				end = GetLength();

			Range subrange(range.origin, end - range.origin);
			array->AddObject(GetSubstring(subrange));

			if(end == GetLength())
				break;

			range.origin = end + 1;
			range.length = GetLength() - range.origin;
		}

		return array->Autorelease();
	}

	String *String::GetPathExtension() const
	{
		Range range(0, GetLength());
		size_t trailing = __GetTrailingPathLocation();
		size_t end = GetLength();

		if(trailing != kRNNotFound)
		{
			range.origin = 0;
			range.length = trailing;

			end = trailing;
		}

		size_t extension = GetRangeOfCharacterInSet(__stringPathExtensionSet, ComparisonMode::Reverse, range).origin;
		if(extension != kRNNotFound)
			return GetSubstring(Range(extension + 1, end - (extension + 1)));

		return nullptr;
	}
	String *String::GetLastPathComponent() const
	{
		Range range(0, GetLength());
		size_t trailing = __GetTrailingPathLocation();
		size_t end = GetLength();

		if(trailing != kRNNotFound)
		{
			range.origin = 0;
			range.length = trailing;

			end = trailing;
		}

		size_t delimiter = GetRangeOfCharacterInSet(__stringPathDelimiterSet, ComparisonMode::Reverse, range).origin;
		if(delimiter != kRNNotFound)
			return GetSubstring(Range(delimiter + 1, end - (delimiter + 1)));

		return Copy();
	}

	String *String::StringByDeletingPathExtension() const
	{
		String *copy = Copy();
		copy->DeletePathExtension();
		return copy->Autorelease();
	}
	String *String::StringByDeletingLastPathComponent() const
	{
		String *copy = Copy();
		copy->DeleteLastPathComponent();
		return copy->Autorelease();
	}
	String *String::StringByAppendingPathComponent(const String *component) const
	{
		String *copy = Copy();
		copy->AppendPathComponent(component);
		return copy->Autorelease();
	}
	String *String::StringByAppendingPathExtension(const String *extension) const
	{
		{
			String *copy = Copy();
			copy->AppendPathExtension(extension);
			return copy->Autorelease();
		}
	}

	
	bool String::WriteToFile(const String *file, Encoding encoding)
	{
		Data *data = GetDataWithEncoding(encoding);
		return data->WriteToFile(file);
	}
}
