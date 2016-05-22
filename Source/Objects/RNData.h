//
//  RNData.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_DATA_H__
#define __RAYNE_DATA_H__

#include "../Base/RNBase.h"
#include "RNObject.h"

namespace RN
{
	class String;
	class Data : public Object
	{
	public:
		RNAPI Data();
		RNAPI Data(const void *bytes, size_t length);
		RNAPI Data(const void *bytes, size_t length, bool noCopy, bool deleteWhenDone);
		RNAPI Data(const Data *other);
		RNAPI Data(Deserializer *deserializer);
		RNAPI ~Data() override;
		
		RNAPI void Serialize(Serializer *serializer) const override;
		
		RNAPI static Data *WithBytes(const uint8 *bytes, size_t length);
		RNAPI static Expected<Data *> WithContentsOfFile(const String *file);

		RNAPI void Append(const void *bytes, size_t length);
		RNAPI void Append(Data *other);
		
		RNAPI void ReplaceBytes(const void *bytes, const Range &range);
		RNAPI bool WriteToFile(const String *file);
		
		RNAPI void GetBytesInRange(void *buffer, Range range) const;
		RNAPI Data *GetDataInRange(Range range) const;
		void *GetBytes() const { return static_cast<void *>(_bytes); }
		size_t GetLength() const { return _length; }
		
	private:
		void Initialize(const void *bytes, size_t length);
		void AssertSize(size_t minimumLength);
		
		uint8 *_bytes;
		size_t _length;
		size_t _allocated;
		
		bool _freeData;
		bool _ownsData;
		
		__RNDeclareMetaInternal(Data)
	};
	
	RNObjectClass(Data)
}

#endif /* __RAYNE_DATA_H__ */
