//
//  RNString.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STRING_H__
#define __RAYNE_STRING_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNUnicode.h"
#include "RNArray.h"

namespace RN
{
	class StringFactory;
	class String : public Object
	{
	public:
		friend class StringFactory;
		
		enum
		{
			ComparisonModeCaseInsensitive = (1 << 0),
			ComparisonModeNumerically = (1 << 1)
		};
		typedef uint32 ComparisonMode;
		
		RNAPI String();
		RNAPI String(const char *string, va_list args);
		RNAPI String(const char *string, bool constant=false);
		RNAPI String(const char *string, size_t length, bool constant=false);
		RNAPI String(const UniChar *string, Encoding encodingHint = Encoding::UTF8);
		RNAPI String(const void *bytes, Encoding encoding, bool constant=false);
		RNAPI String(const void *bytes, size_t length, Encoding encoding, bool constant=false);
		RNAPI String(const String *string);
		RNAPI String(Deserializer *deserializer);
		RNAPI ~String() override;
		
		RNAPI void Serialize(Serializer *serializer) override;
		
		RNAPI static String *WithFormat(const char *string, ...);
		RNAPI static String *WithString(const char *string, bool constant=false);
		RNAPI static String *WithString(const char *string, size_t length, bool constant=false);
		RNAPI static String *WithUnicode(const UniChar *string, Encoding encodingHint = Encoding::UTF8);
		RNAPI static String *WithBytes(const void *bytes, Encoding encoding, bool constant=false);
		RNAPI static String *WithBytes(const void *bytes, size_t length, Encoding encoding, bool constant=false);
		RNAPI static String *WithContentsOfFile(const std::string& file, Encoding encoding);
		
		RNAPI machine_hash GetHash() const override;
		RNAPI bool IsEqual(Object *other) const override;
		
		RNAPI void Append(const String *string);
		RNAPI void Append(const char *string, ...);
		RNAPI void Insert(const String *string, size_t index);
		RNAPI void Capitalize();
		
		RNAPI void DeleteCharacters(const Range& range);
		
		RNAPI void ReplaceCharacters(const String *replacement, const Range& range);
		RNAPI void ReplaceOccurrencesOfString(const String *string, const String *replacement);
		
		RNAPI Range GetRangeOfString(const String *string, ComparisonMode mode=0) const;
		RNAPI Range GetRangeOfString(const String *string, ComparisonMode mode, const Range& range) const;
		
		RNAPI ComparisonResult Compare(const String *other, ComparisonMode mode=0) const;
		RNAPI ComparisonResult Compare(const String *other, ComparisonMode mode, const Range& range) const;
		
		RNAPI String *GetSubstring(const Range& range) const;
		RNAPI UniChar GetCharacterAtIndex(size_t index) const;
		RNAPI Array *GetComponentsSeparatedByString(const String *other) const;
		
		RNAPI size_t GetLength() const;
		
		RNAPI uint8 *GetBytesWithEncoding(Encoding encoding, bool lossy, size_t *length) const;
		RNAPI char *GetUTF8String() const;
		
	private:
		String(void *internal);
		void PromoteStringIfNeeded(Encoding encoding);
		
		Encoding _encoding;
		void *_internal; // Of abstract type BasicString
		
		RNDeclareMeta(String)
	};
}

#define RNSTR(...)  RN::String::WithFormat(__VA_ARGS__)
#define RNCSTR(cstr) RN::String::WithString(cstr, true)

#define RNUTF8STR(str)  RN::String::WithBytes(str, RN::Encoding::UTF8)
#define RNCUTF8STR(str) RN::String::WithBytes(str, RN::Encoding::UTF8, true)

#endif /* __RAYNE_STRING_H__ */
