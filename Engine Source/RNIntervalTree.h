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
		class Interval
		{
		public:
			Interval(const Range& trange, const T& tvalue) :
				range(trange),
				value(tvalue)
			{}
			
			Range range;
			T value;
		};

		template <class T>
		class IntervalTree
		{
		public:
			
			IntervalTree()
			{
				_center = 0;
				_nodes[0] = _nodes[1] = nullptr;
			}
			
			IntervalTree(const IntervalTree<T>& other) :
				_intervals(other._intervals)
			{
				_center = other._center;
				
				for(size_t i=0; i<2; i++)
				{
					_nodes[i] = (other._nodes[i]) ? new IntervalTree<T>(*other._nodes[i]) : nullptr;
				}
			}
			
			IntervalTree(const std::vector<Interval<T>>& intervals, uint32 depth=16, uint32 minbucket=64, int leftextent=0, int rightextent=0, uint32 maxbucket=512) :
				IntervalTree(std::move(std::vector<Interval<T>>(intervals)), depth, minbucket, leftextent, rightextent, maxbucket)
			{}
			
			IntervalTree(std::vector<Interval<T>>&& intervals, uint32 depth=16, uint32 minbucket=64, int leftextent=0, int rightextent=0, uint32 maxbucket=512)
			{
				_center = 0;
				_nodes[0] = _nodes[1] = nullptr;
				
				depth --;
				
				if(depth == 0 || (intervals.size() < minbucket && intervals.size() < maxbucket))
				{
					_intervals = std::move(intervals);
				}
				else
				{				
					if(leftextent == 0 && rightextent == 0)
					{
						ParallelSort(intervals.begin(), intervals.end(), [](const Interval<T>& a, const Interval<T>& b) {
							return a.range.origin < b.range.origin;
						});
						
						std::vector<machine_uint> stops;
						stops.resize(intervals.size());
						std::transform(intervals.begin(), intervals.end(), stops.begin(), [](const Interval<T>& interval) {
							return interval.range.End();
						});
						
						leftextent  = static_cast<int>(intervals.front().range.origin);
						rightextent = static_cast<int>(*std::max_element(stops.begin(), stops.end()));
					}
					
					_center = static_cast<int>(intervals.at(intervals.size() / 2).range.origin);
					
					std::vector<Interval<T>> left;
					std::vector<Interval<T>> right;
					
					for(auto i=intervals.begin(); i!=intervals.end(); i++)
					{
						if(i->range.End() < _center)
						{
							left.push_back(std::move(*i));
						}
						else if(i->range.End() > _center)
						{
							right.push_back(std::move(*i));
						}
						else
						{
							_intervals.push_back(std::move(*i));
						}
					}

					if(!left.empty())
					{
						_nodes[0] = new IntervalTree<T>(left, depth, minbucket, leftextent, _center);
					}
					
					if(!right.empty())
					{
						_nodes[1] = new IntervalTree<T>(right, depth, minbucket, rightextent, _center);
					}
				}
				
			}
			
			~IntervalTree()
			{
				delete _nodes[0];
				delete _nodes[1];
			}
			
			IntervalTree& operator=(const IntervalTree<T>& other)
			{
				_center = other._center;
				_intervals = other._intervals;
				
				for(size_t i=0; i<2; i++)
				{
					_nodes[i] = (other._nodes[i]) ? new IntervalTree<T>(*other._nodes[i]) : nullptr;
				}
				
				return *this;
			}
					
					
			void FindContained(const Range& range, std::vector<Interval<T>>& contained)
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
					_nodes[0]->FindContained(range, contained);
				
				if(_nodes[1] && range.End() >= _center)
					_nodes[1]->FindContained(range, contained);
			}
					
			void FindOverlapping(const Range& range, std::vector<Interval<T>>& overlapping)
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
					_nodes[0]->FindOverlapping(range, overlapping);
				
				if(_nodes[1] && range.End() >= _center)
					_nodes[1]->FindOverlapping(range, overlapping);
			}
			
		private:
			IntervalTree<T> *_nodes[2];
			std::vector<Interval<T>> _intervals;
			int _center;
		};
	}
}

#endif /* __RAYNE_INTERVALTREE_H__ */
