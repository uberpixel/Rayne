//
//  RNASCIIString.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.//

#ifndef __RAYNE_ASCIISTRING_H__
#define __RAYNE_ASCIISTRING_H__

#include "RNBase.h"
#include "RNBasicString.h"

namespace RN
{
	class ConstantASCIIString : public BasicString
	{
	public:
		ConstantASCIIString(const char *string);
		
		BasicString *SimpleCopy() const override;
		BasicString *MutableCopy() const override;
		
		UniChar CharacterAtIndex(size_t index) const override;
		void CharactersInRange(UniChar *buffer, const Range& range) const override;
		
		size_t Length() const override;
		machine_hash Hash() const override;
		
		void *Data() const override;
		void *BytesWithEncoding(Encoding encoding, bool lossy, size_t *length) const override;
		bool IsMutable() const override;
		BasicString *Substring(const Range& range) const override;
		Encoding CharacterEncoding() const override;
		
	protected:
		ConstantASCIIString();
		
		void UpdateHash();
		
		char *_string;
		size_t _length;
		machine_hash _hash;
		
		RNDeclareMeta(ConstantASCIIString, BasicString)
	};
	
	class ASCIIString : public ConstantASCIIString
	{
	public:
		ASCIIString(const char *string);
		ASCIIString(const char *string, size_t length);
		ASCIIString(UniChar *string);
		~ASCIIString() override;
		
		BasicString *SimpleCopy() const override;
		
		void ReplaceCharactersInRange(const Range& range, BasicString *string) override;
		bool IsMutable() const override;
		
	private:
		void AllocateSpace(size_t size);
		size_t _size;
		
		RNDeclareMeta(ASCIIString, ConstantASCIIString)
	};
}

#endif /* __RAYNE_ASCIISTRING_H__ */
