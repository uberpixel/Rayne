//
//  RNIntervalTree.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INTERVALTREE_H__
#define __RAYNE_INTERVALTREE_H__

#include "RNBase.h"
#include "RNAlgorithm.h"

// Original implementation: https://github.com/ekg/intervaltree

namespace RN
{
	namespace stl
	{
		template <class T>
		class interval_tree
		{
		public:
			class interval
			{
			public:
				interval(const Range& trange, const T& tvalue) :
					range(trange),
					value(tvalue)
				{}
				
				Range range;
				T value;
			};
			
			interval_tree()
			{
				_center = 0;
				_nodes[0] = _nodes[1] = nullptr;
				_size = 0;
			}
			
			interval_tree(const interval_tree<T>& other) :
				_intervals(other._intervals)
			{
				_center      = other._center;
				_leftextent  = other._leftextent;
				_rightextent = other._rightextent;
				_size        = other._size;
				
				for(size_t i=0; i<2; i++)
				{
					_nodes[i] = (other._nodes[i]) ? new interval_tree<T>(*other._nodes[i]) : nullptr;
				}
			}
			
			interval_tree(const std::vector<interval>& intervals, uint32 depth=16, uint32 minbucket=64, int32 leftextent=0, int32 rightextent=0, uint32 maxbucket=512) :
				interval_tree(std::move(std::vector<interval>(intervals)), depth, minbucket, leftextent, rightextent, maxbucket)
			{}
			
			interval_tree(std::vector<interval>&& intervals, uint32 depth=16, uint32 minbucket=64, int32 leftextent=0, int32 rightextent=0, uint32 maxbucket=512)
			{
				if(leftextent == 0 && rightextent == 0)
				{
					ParallelSort(intervals.begin(), intervals.end(), [](const interval& a, const interval& b) {
						return a.range.origin < b.range.origin;
					});
					
					std::vector<size_t> stops;
					stops.resize(intervals.size());
					std::transform(intervals.begin(), intervals.end(), stops.begin(), [](const interval& interval) {
						return interval.range.GetEnd();
					});
					
					_leftextent  = static_cast<int>(intervals.front().range.origin);
					_rightextent = static_cast<int>(*std::max_element(stops.begin(), stops.end()));
				}
				
				_center = static_cast<int>(intervals.at(intervals.size() / 2).range.origin);
				_nodes[0] = _nodes[1] = nullptr;
				
				if((--depth) == 0 || (intervals.size() < minbucket && intervals.size() < maxbucket))
				{
					_intervals = std::move(intervals);
				}
				else
				{
					std::vector<interval> left;
					std::vector<interval> right;
					
					for(auto i=intervals.begin(); i!=intervals.end(); i++)
					{
						if(i->range.GetEnd() < _center)
						{
							left.push_back(std::move(*i));
						}
						else if(i->range.GetEnd() > _center)
						{
							right.push_back(std::move(*i));
						}
						else
						{
							_intervals.push_back(std::move(*i));
						}
					}

					if(!left.empty())
						_nodes[0] = new interval_tree<T>(left, depth, minbucket, _leftextent, _center);
					
					if(!right.empty())
						_nodes[1] = new interval_tree<T>(right, depth, minbucket, _rightextent, _center);
				}
				
				_size = _intervals.size();
			}
			
			~interval_tree()
			{
				delete _nodes[0];
				delete _nodes[1];
			}
			
			interval_tree& operator=(const interval_tree<T>& other)
			{
				_center      = other._center;
				_leftextent  = other._leftextent;
				_rightextent = other._rightextent;
				_intervals   = other._intervals;
				
				for(size_t i=0; i<2; i++)
				{
					_nodes[i] = (other._nodes[i]) ? new interval_tree<T>(*other._nodes[i]) : nullptr;
				}
				
				return *this;
			}
			
			int32 min() const
			{
				return _leftextent;
			}
					
			int32 max() const
			{
				return _rightextent;
			}
			
			size_t size() const
			{
				return _size;
			}
			
			void find_contained(const Range& range, std::vector<interval>& contained)
			{
				if(!_intervals.empty())
				{
					for(auto i=_intervals.begin(); i!=_intervals.end(); i++)
					{
						if(i->range.Contains(range))
						{
							contained.push_back(*i);
						}
					}
				}
				
				if(_nodes[0] && range.origin <= _center)
					_nodes[0]->find_contained(range, contained);
				
				if(_nodes[1] && range.GetEnd() >= _center)
					_nodes[1]->find_contained(range, contained);
			}
					
			void find_overlapping(const Range& range, std::vector<interval>& overlapping)
			{
				if(!_intervals.empty())
				{
					for(auto i=_intervals.begin(); i!=_intervals.end(); i++)
					{
						if(i->range.Overlaps(range))
						{
							overlapping.push_back(*i);
						}
					}
				}
				
				if(_nodes[0] && range.origin <= _center)
					_nodes[0]->find_overlapping(range, overlapping);
				
				if(_nodes[1] && range.GetEnd() >= _center)
					_nodes[1]->find_overlapping(range, overlapping);
			}
			
		private:
			interval_tree<T> *_nodes[2];
			std::vector<interval> _intervals;
			int32 _center;
			int32 _leftextent, _rightextent;
			
			size_t _size;
		};
	}
}

#endif /* __RAYNE_INTERVALTREE_H__ */
