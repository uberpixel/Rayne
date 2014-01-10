//
//  RNSpatialMap.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SPATIALHASH_H__
#define __RAYNE_SPATIALHASH_H__

#include "RNBase.h"
#include "RNVector.h"
#include "RNMath.h"

#define P1 1149851
#define P2 1860498
#define P3 87403803
#define P4 1568397607
#define P5 3010349

namespace RN
{
	namespace stl
	{
		struct spatial_hasher
		{
			size_t operator() (const Vector3 &vector) const
			{
				int32 x = (int32)vector.x;
				int32 z = (int32)vector.z;
				size_t h = (x * z) + P5;
				
				h ^= (x < 0) ? x * P1 : x * P2;
				h ^= (z < 0) ? z * P3 : z * P4;
				
				return h;
			}
		};
		
		template<class T, size_t SpacingXY = 16, size_t SpacingY = 8>
		class spatial_map
		{
		public:
			
			void insert(const Vector3 &position, const T &val)
			{
				(*this)[position] = val;
			}
			
			void erase(const Vector3 &position)
			{
				Vector3 translated(std::move(translate_vector(position)));
				auto iterator = _entries.find(static_cast<int32>(translated.y));
				
				if(iterator != _entries.end())
				{
					auto &map = iterator.second;
					map.erase(translated);
					
					if(map.empty())
						_entries.erase(static_cast<int32>(translated.y));
				}
			}
			
			T& operator[](const Vector3 &key)
			{
				Vector3 translated(std::move(translate_vector(key)));
				auto iterator = _entries.find(static_cast<int32>(translated.y));
				
				if(iterator != _entries.end())
				{
					auto &map = iterator->second;
					return map[translated];
				}
				
				std::unordered_map<Vector3, T, spatial_hasher> map;
				T &value = map[translated];
				
				_entries.emplace(static_cast<int32>(translated.y), std::move(map));
				
				return value;
			}
			
		private:
			Vector3 translate_vector(const Vector3 &vector)
			{
				Vector3 result;
				
				result.x = vector.x / SpacingXY;
				result.y = vector.y / SpacingY;
				result.z = vector.z / SpacingXY;
				
				result.x = (result.x >= 0.0f) ? ceilf(result.x) : floorf(result.x);
				result.y = (result.y >= 0.0f) ? ceilf(result.y) : floorf(result.y);
				result.z = (result.z >= 0.0f) ? ceilf(result.z) : floorf(result.z);
				
				return result;
			}
			
			std::unordered_map<int32, std::unordered_map<Vector3, T, spatial_hasher>> _entries;
		};
	}
}

#undef P1
#undef P2
#undef P3
#undef P4
#undef P5

#endif /* __RAYNE_SPATIALHASH_H__ */
