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
#include "RNCharacterSet.h"
#include "../Math/RNVector.h"

namespace RN
{
	class UTF8String;
	class Data;

	class String : public Object
	{
	public:
		RN_OPTIONS(ComparisonMode, uint32,
		           CaseInsensitive = (1 << 0),
		           Numerically = (1 << 1),
				   Reverse = (1 << 2));

		RNAPI static void InitialWakeUp(MetaClass *cls);

		RNAPI String();
		RNAPI String(const char *string, va_list args);
		RNAPI String(const char *string, bool constant=false);
		RNAPI String(const char *string, size_t length, bool constant=false);
		RNAPI String(const void *bytes, Encoding encoding, bool constant=false);
		RNAPI String(const void *bytes, size_t length, Encoding encoding, bool constant=false);
		RNAPI String(const Data *data, Encoding encoding);
		RNAPI String(const String *string);
		RNAPI String(Deserializer *deserializer);
		RNAPI ~String() override;
		
		RNAPI void Serialize(Serializer *serializer) const override;
		
		RNAPI static String *WithFormat(const char *string, ...);
		RNAPI static String *WithString(const char *string, bool constant = false);
		RNAPI static String *WithString(const char *string, size_t length, bool constant = false);
		RNAPI static String *WithBytes(const void *bytes, Encoding encoding, bool constant = false);
		RNAPI static String *WithBytes(const void *bytes, size_t length, Encoding encoding, bool constant = false);
		RNAPI static Expected<String *> WithContentsOfFile(const String *file, Encoding encoding);
		
		RNAPI size_t GetHash() const override;
		RNAPI bool IsEqual(const Object *other) const override;
		RNAPI const String *GetDescription() const override;

		/**
		 * Mutation
		 **/
		RNAPI void Append(const String *string);
		RNAPI void Append(const char *string, ...);
		RNAPI void Insert(const String *string, size_t index);
		RNAPI void Capitalize();
		RNAPI void MakeUppercase();
		RNAPI void MakeLowercase();
		
		RNAPI void DeleteCharacters(const Range &range);
		
		RNAPI void ReplaceCharacters(const String *replacement, const Range &range);
		RNAPI void ReplaceOccurrencesOfString(const String *string, const String *replacement);

		RNAPI String *StringByAppendingString(const String *other) const;

		/**
		 * Comparison / Lookup
		 **/
		RNAPI Range GetRangeOfString(const String *string, ComparisonMode mode = 0) const;
		RNAPI Range GetRangeOfString(const String *string, ComparisonMode mode, const Range &range) const;

		RNAPI Range GetRangeOfCharacterInSet(const CharacterSet *set, ComparisonMode mode = 0) const;
		RNAPI Range GetRangeOfCharacterInSet(const CharacterSet *set, ComparisonMode mode, const Range &range) const;

		RNAPI ComparisonResult Compare(const String *other, ComparisonMode mode = 0) const;
		RNAPI ComparisonResult Compare(const String *other, ComparisonMode mode, const Range &range) const;

		RNAPI bool HasPrefix(const String *other) const;
		RNAPI bool HasSuffix(const String *other) const;
		
		RNAPI String *GetSubstring(const Range &range) const;
		RNAPI UniChar GetCharacterAtIndex(size_t index) const;
		RNAPI Array *GetComponentsSeparatedByString(const String *other) const;
		
		RNAPI size_t GetLength() const;

		/**
		 * Raw access
		 **/
		RNAPI uint8 *GetBytesWithEncoding(Encoding encoding, bool lossy, size_t &length) const;
		RNAPI Data *GetDataWithEncoding(Encoding encoding) const;
		RNAPI char *GetUTF8String() const;

		/**
		 * Paths
		 **/
		RNAPI void DeletePathExtension();
		RNAPI void DeleteLastPathComponent();
		RNAPI void AppendPathComponent(const String *component);
		RNAPI void AppendPathExtension(const String *extension);
		RNAPI String *StringByDeletingPathExtension() const;
		RNAPI String *StringByDeletingLastPathComponent() const;
		RNAPI String *StringByAppendingPathComponent(const String *component) const;
		RNAPI String *StringByAppendingPathExtension(const String *extension) const;
		RNAPI Array *GetPathComponents() const;
		RNAPI String *GetPathExtension() const;
		RNAPI String *GetLastPathComponent() const;

		RNAPI String *GetNormalizedPath() const;


		RNAPI void WriteToFile(const String *file, Encoding encoding);
		
	private:
		size_t __GetTrailingPathLocation() const;
		void __DeleteTrailingPath();

		String(UTF8String *string);
		UTF8String *_string;
		
		__RNDeclareMetaInternal(String)
	};

	RNObjectClass(String)

	class StringBuilder
	{
	public:
		StringBuilder()
		{}

		String *Build() const
		{
			std::string string = _stream.str();
			return String::WithString(string.c_str(), false);
		}

		StringBuilder &operator << (const Object *object) { if(object) { _stream << object->GetDescription()->GetUTF8String(); } else { _stream << "<null>"; } return *this; }
		StringBuilder &operator << (const std::exception &e) { _stream << e.what(); return *this; }
		StringBuilder &operator << (const std::string &val) { _stream << val; return *this; }
		StringBuilder &operator << (const char *val) { _stream << val; return *this; }
		StringBuilder &operator << (bool val) { _stream << val; return *this; }
		StringBuilder &operator << (short val) { _stream << val; return *this; }
		StringBuilder &operator << (unsigned short val) { _stream << val; return *this; }
		StringBuilder &operator << (int val) { _stream << val; return *this; }
		StringBuilder &operator << (unsigned int val) { _stream << val; return *this; }
		StringBuilder &operator << (long val) { _stream << val; return *this; }
		StringBuilder &operator << (unsigned long val) { _stream << val; return *this; }
		StringBuilder &operator << (long long val) { _stream << val; return *this; }
		StringBuilder &operator << (unsigned long long val) { _stream << val; return *this; }
		StringBuilder &operator << (float val) { _stream << val; return *this; }
		StringBuilder &operator << (double val) { _stream << val; return *this; }
		StringBuilder &operator << (long double val) { _stream << val; return *this; }
		StringBuilder &operator << (const void *val) { _stream << val; return *this; }
		StringBuilder &operator << (std::ostream &(*pf)(std::ostream &)) { _stream << pf; return *this; }
		StringBuilder &operator << (std::ios &(*pf)(std::ios &)) { _stream << pf; return *this; };
		StringBuilder &operator << (std::ios_base &(*pf)(std::ios_base &)) { _stream << pf; return *this; }
		StringBuilder &operator << (const RN::Vector2 &val) { _stream << "(" << val.x << ", " << val.y << ")"; return *this; }
		StringBuilder &operator << (const RN::Vector3 &val) { _stream << "(" << val.x << ", " << val.y << ", " << val.z << ")"; return *this; }
		StringBuilder &operator << (const RN::Vector4 &val) { _stream << "(" << val.x << ", " << val.y << ", " << val.z << ", " << val.w << ")"; return *this; }
		StringBuilder &operator << (const RN::Quaternion &val) { _stream << "(" << val.x << ", " << val.y << ", " << val.z << ", " << val.w << ")"; return *this; }

	private:
		std::stringstream _stream;
	};

	template<class T, size_t N>
	String *__MakeConstantString(T (&cstr)[N])
	{
		return String::WithString(cstr, N, true);
	}
}

#define RNSTR(...) (RN::StringBuilder{} << __VA_ARGS__).Build()
#define RNSTRF(...)  RN::String::WithFormat(__VA_ARGS__)
#define RNCSTR(cstr) RN::__MakeConstantString(cstr)

#define RNUTF8STR(str)  RN::String::WithBytes(str, RN::Encoding::UTF8)
#define RNCUTF8STR(str) RN::String::WithBytes(str, RN::Encoding::UTF8, true)

#endif /* __RAYNE_STRING_H__ */
