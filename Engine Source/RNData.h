//
//  RNData.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_DATA_H__
#define __RAYNE_DATA_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Data : public Object
	{
	public:
		Data();
		Data(const void *bytes, size_t length);
		Data(const void *bytes, size_t length, bool noCopy, bool deleteWhenDone);
		Data(const std::string& file);
		Data(const Data& other);
		Data(Data *other);
		~Data() override;
		
		static Data *WithBytes(const uint8 *bytes, size_t length);
		static Data *WithContentsOfFile(const std::string& file);

		void Append(const void *bytes, size_t length);
		void Append(Data *other);
		
		void ReplaceBytes(const void *bytes, const Range& range);
		void WriteToFile(const std::string& file);
		
		void GetBytesInRange(void *buffer, Range range) const;
		Data *GetDataInRange(Range range) const;
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
		
		RNDefineMetaWithTraits(Data, Object, MetaClassTraitCronstructable, MetaClassTraitCopyable)
	};
}

#endif /* __RAYNE_DATA_H__ */
