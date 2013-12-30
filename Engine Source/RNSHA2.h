//
//  RNSHA2.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SHA2_H__
#define __RAYNE_SHA2_H__

#include "RNBase.h"

namespace RN
{
	namespace stl
	{
		class sha2_context
		{
		public:
			RNAPI sha2_context();
			
			RNAPI void update(const std::vector<uint8>& input);
			RNAPI void update(const uint8 *input, size_t size);
			
			RNAPI void finish(std::vector<uint8>& result);
			
		private:
			void process(const uint8 *data);
			
			uint64 _total;
			uint32 _state[8];
			uint8 _buffer[64];
		};
		
		static inline void sha2(const uint8 *input, size_t length, std::vector<uint8>& result)
		{
			sha2_context context;
			context.update(input, length);
			context.finish(result);
		}
	}
}

#endif /* __RAYNE_SHA2_H__ */
