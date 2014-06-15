//
//  RNStringInternal.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STRINGINTERNAL_H__
#define __RAYNE_STRINGINTERNAL_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNUnicode.h"
#include "RNEnum.h"

namespace RN
{
	class UTF8String;

	class StringPool
	{
	public:
		static UTF8String *CreateUTF8String(const void *string);
	};

	class UTF8String : public Object
	{
	public:
		friend class StringPool;

		struct Flags : public Enum<uint32>
		{
		public:
			Flags()
			{}
			
			Flags(uint32 val) :
				Enum(val)
			{}
			
			enum
			{
				ByteCountMask = 0xfff,
				
				ConstStorage = (1 << 16),
				ASCII = (1 << 17)
			};
		};
		
		UTF8String();
		UTF8String(const UTF8String *other);
		UTF8String(const uint8 *data, size_t bytes, bool mutableStorage);
		UTF8String(const void *data, size_t bytes, Encoding encoding, bool mutableStorage);
		
		UTF8String *MutableCopy() const;
		
		machine_hash GetHash() const { const_cast<UTF8String *>(this)->RecalcuateHash(); return _hash; }
		size_t GetLength() const { return _length; }
		bool IsMutable() const { return !(_flags & Flags::ConstStorage); }
		
		const uint8 *GetBytes() const;
		size_t GetBytesCount() const;
		
		UniChar GetCharacterAtIndex(size_t index) const;
		void GetCharactersInRange(UniChar *buffer, const Range &range) const;
		
		void *GetBytesWithEncoding(Encoding encoding, bool lossy, size_t &length) const;
		UTF8String *GetSubstring(const Range &range) const;
		
		
		void ReplaceCharactersInRange(const Range &range, UTF8String *string);
		bool ValidateUTF8() const;
		
	protected:
		UTF8String(const uint8 *storage, size_t length, machine_hash hash, Flags flags);

		size_t SkipCharacters(const uint8 *string, size_t skip) const;
		void WakeUpWithUTF8String(const uint8 *data, size_t count, bool mutableStorage);
		
		void RecalcuateHash();
		void MakeMutable();
		
		std::vector<uint8> _storage;
		const uint8 *_constStorage;
		
		size_t _length;
		machine_hash _hash;
		
		Flags _flags;
		
		RNDeclareMeta(UTF8String)
	};
}

#endif /* __RAYNE_STRINGINTERNAL_H__ */
