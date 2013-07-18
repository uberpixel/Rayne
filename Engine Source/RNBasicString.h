//
//  RNBasicString.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BASICSTRING_H__
#define __RAYNE_BASICSTRING_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNUnicode.h"

namespace RN
{
	enum class StringTraits
	{
		Constant,
		Mutable
	};
	
	class BasicString : public Object
	{
	public:
		virtual ~BasicString();
		virtual BasicString *Copy() const;
		virtual BasicString *MutableCopy() const;
		
		virtual UniChar CharacterAtIndex(size_t index) const;
		virtual void CharactersInRange(UniChar *buffer, const Range& range) const;
		
		virtual void ReplaceCharactersInRange(const Range& range, BasicString *string);
		
		virtual machine_hash Hash() const;
		virtual size_t Length() const;
		
		virtual void *Data() const;
		virtual void *BytesWithEncoding(Encoding encoding, bool lossy, size_t *length) const;
		virtual BasicString *Substring(const Range& range) const;
		virtual bool IsMutable() const;
		
		virtual Encoding CharacterEncoding() const;
		
		RNDefineMeta(BasicString, Object)
	};
	
	class StringFactory
	{
	public:
		static BasicString *ConstructString(const void *data, Encoding encoding, StringTraits traits);
		static BasicString *ConstructString(const void *data, size_t length, Encoding encoding, StringTraits traits);
		static BasicString *ConstructString(BasicString *string, Encoding encoding);
		static BasicString *EmptyString();
		
	private:
		static BasicString *DequeConstantString(void *data, Encoding encoding);
		
		static std::unordered_map<void *, BasicString *> _stringTable;
		static SpinLock _stringTableLock;
	};
}

#endif /* __RAYNE_BASICSTRING_H__ */
