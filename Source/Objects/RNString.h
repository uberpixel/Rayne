//
//  RNString.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STRING_H__
#define __RAYNE_STRING_H__

#include "../Base/RNBase.h"
#include "../Base/RNUnicode.h"
#include "RNObject.h"
#include "RNArray.h"

namespace RN
{
	class UTF8String;
	class String : public Object
	{
	public:
		RN_OPTIONS(ComparisonMode, uint32,
		           CaseInsensitive = (1 << 0),
		           Numerically = (1 << 1));

		RNAPI String();
		RNAPI String(const char *string, va_list args);
		RNAPI String(const char *string, bool constant=false);
		RNAPI String(const char *string, size_t length, bool constant=false);
		RNAPI String(const void *bytes, Encoding encoding, bool constant=false);
		RNAPI String(const void *bytes, size_t length, Encoding encoding, bool constant=false);
		RNAPI String(const String *string);
		RNAPI String(Deserializer *deserializer);
		RNAPI ~String() override;
		
		RNAPI void Serialize(Serializer *serializer) override;
		
		RNAPI static String *WithFormat(const char *string, ...);
		RNAPI static String *WithString(const char *string, bool constant = false);
		RNAPI static String *WithString(const char *string, size_t length, bool constant = false);
		RNAPI static String *WithBytes(const void *bytes, Encoding encoding, bool constant = false);
		RNAPI static String *WithBytes(const void *bytes, size_t length, Encoding encoding, bool constant = false);
		RNAPI static String *WithContentsOfFile(const std::string &file, Encoding encoding);
		
		RNAPI size_t GetHash() const override;
		RNAPI bool IsEqual(Object *other) const override;
		
		RNAPI void Append(const String *string);
		RNAPI void Append(const char *string, ...);
		RNAPI void Insert(const String *string, size_t index);
		RNAPI void Capitalize();
		
		RNAPI void DeleteCharacters(const Range &range);
		
		RNAPI void ReplaceCharacters(const String *replacement, const Range &range);
		RNAPI void ReplaceOccurrencesOfString(const String *string, const String *replacement);
		
		RNAPI Range GetRangeOfString(const String *string, ComparisonMode mode = 0) const;
		RNAPI Range GetRangeOfString(const String *string, ComparisonMode mode, const Range &range) const;
		
		RNAPI ComparisonResult Compare(const String *other, ComparisonMode mode = 0) const;
		RNAPI ComparisonResult Compare(const String *other, ComparisonMode mode, const Range &range) const;
		
		RNAPI String *GetSubstring(const Range &range) const;
		RNAPI UniChar GetCharacterAtIndex(size_t index) const;
		RNAPI Array *GetComponentsSeparatedByString(const String *other) const;
		
		RNAPI size_t GetLength() const;
		
		RNAPI uint8 *GetBytesWithEncoding(Encoding encoding, bool lossy, size_t &length) const;
		RNAPI char *GetUTF8String() const;
		
		RNAPI bool WriteToFile(const std::string &file, Encoding encoding);
		
	private:
		String(UTF8String *string);
		UTF8String *_string;
		
		RNDeclareMeta(String)
	};
}

#define RNSTR(...)  RN::String::WithFormat(__VA_ARGS__)
#define RNCSTR(cstr) RN::String::WithString(cstr, true)

#define RNUTF8STR(str)  RN::String::WithBytes(str, RN::Encoding::UTF8)
#define RNCUTF8STR(str) RN::String::WithBytes(str, RN::Encoding::UTF8, true)

#endif /* __RAYNE_STRING_H__ */
