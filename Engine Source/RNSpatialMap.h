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
#include "RNAABB.h"

#define P1 1149851
#define P2 1860498
#define P3 87403803
#define P4 1568397607
#define P5 3010349
#define P6 83492791
#define P7 73856093

namespace RN
{
	namespace stl
	{
		struct spatial_hasher
		{
			size_t operator() (const Vector3 &vector) const
			{
				int32 x = static_cast<int32>(vector.x);
				int32 y = static_cast<int32>(vector.y);
				int32 z = static_cast<int32>(vector.z);
				size_t h = (x * y * z) + P5;
				
				h ^= (x < 0) ? x * P1 : x * P2;
				h ^= (z < 0) ? z * P3 : z * P4;
				h ^= (y < 0) ? y * P6 : y * P7;
				
				return h;
			}
		};
		
		template<class T>
		class spatial_map
		{
		public:
			spatial_map(size_t spacingXZ, bool spacingY) :
				_spacing(spacingXZ),
				_spacingY(spacingY)
			{}
			
			void insert(const Vector3 &position, const T &val)
			{
				(*this)[position] = val;
			}
			
			void erase(const Vector3 &position)
			{
				_entries.erase(position);
			}
			
			T &operator[](const Vector3 &key)
			{
				Vector3 translated(std::move(translate_vector(key)));
				return _entries[translated];
			}
			
			T &access(const Vector3 &key, const Vector3 &offset = Vector3())
			{
				Vector3 translated(std::move(translate_vector(key)));
				return _entries[translated + offset];
			}
			
			bool contains(const Vector3 &key, const Vector3 &offset = Vector3())
			{
				Vector3 translated(std::move(translate_vector(key)));
				return raw_contains(translated + offset);
			}
			
			void query(const AABB &aabb, std::vector<T> &result)
			{
				Vector3 position(std::move(translate_vector(aabb.position)));
				Vector3 extents(aabb.maxExtend - aabb.minExtend);
				
				extents /= Vector3(_spacing, _spacingY ? _spacing : 1.0, _spacing);
				extents *= 0.5f;
				
				extents.x = ceilf(extents.x);
				extents.y = ceilf(extents.y);
				extents.z = ceilf(extents.z);
				
				for(float x = -extents.x; x <= extents.x; x ++)
				{
					for(float z = -extents.z; z <= extents.z; z ++)
					{
						if(_spacingY)
						{
							for(float y = -extents.y; y <= extents.z; y ++)
							{
								if(raw_contains(position + Vector3(x, y, z)))
									result.push_back(_entries[position + Vector3(x, y, z)]);
							}
						}
						else
						{
							if(raw_contains(position + Vector3(x, 0.0f, z)))
								result.push_back(_entries[position + Vector3(x, 0.0f, z)]);
						}
					}
				}
			}
			
			void set_spacing(size_t spacing, bool spacingY)
			{
				if(spacing != _spacing || spacingY != _spacingY)
				{
					_spacing = spacing;
					_spacingY = spacingY;
					
					_entries.clear();
				}
			}
			
			void clear()
			{
				_entries.clear();
			}
			
			size_t get_spacing() const
			{
				return _spacing;
			}
			
			Vector3 translate_vector(const Vector3 &vector)
			{
				Vector3 result;
				
				result.x = roundf(vector.x / _spacing);
				result.y = _spacingY ? roundf(vector.y / _spacing) : 0.0f;
				result.z = roundf(vector.z / _spacing);
				
				return result;
			}
			
		private:
			bool raw_contains(const Vector3 &position)
			{
				return (_entries.find(position) != _entries.end());
			}
			
			size_t _spacing;
			bool _spacingY;
			
			std::unordered_map<Vector3, T, spatial_hasher> _entries;
		};
	}
}

#undef P1
#undef P2
#undef P3
#undef P4
#undef P5
#undef P6
#undef P7

#endif /* __RAYNE_SPATIALHASH_H__ */
