//
//  RNUTF8String.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UTF8STRING_H__
#define __RAYNE_UTF8STRING_H__

#include "RNBase.h"
#include "RNBasicString.h"

namespace RN
{
	class ConstantUTF8String : public BasicString
	{
	public:
		ConstantUTF8String(const uint8 *string);
		
		BasicString *SimpleCopy() const override;
		BasicString *MutableCopy() const override;
		
		UniChar CharacterAtIndex(size_t index) const override;
		void CharactersInRange(UniChar *buffer, const Range& range) const override;
		
		size_t Length() const override;
		size_t Size() const { return _size; }
		machine_hash Hash() const override;
		
		void *Data() const override;
		void *BytesWithEncoding(Encoding encoding, bool lossy, size_t *length) const override;
		bool IsMutable() const override;
		BasicString *Substring(const Range& range) const override;
		Encoding CharacterEncoding() const override;
		
	protected:
		ConstantUTF8String();
		ConstantUTF8String(const uint8 *string, size_t size);
		
		void UpdateHash();
		
		uint8 *_string;
		size_t _length;
		size_t _size;
		machine_hash _hash;
		
		RNDeclareMeta(ConstantUTF8String)
	};
	
	class UTF8String : public ConstantUTF8String
	{
	public:
		UTF8String(const uint8 *bytes);
		UTF8String(const uint8 *bytes, size_t size);
		UTF8String(UniChar *string);
		
		~UTF8String() override;
		
		BasicString *SimpleCopy() const override;
		
		void ReplaceCharactersInRange(const Range& range, BasicString *string) override;
		bool IsMutable() const override;
		
	private:
		void AllocateSpace(size_t size);
		size_t _allocated;
		
		RNDeclareMeta(UTF8String)
	};
}

#endif /* __RAYNE_UTF8STRING_H__ */
