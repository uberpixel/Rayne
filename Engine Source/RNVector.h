//
//  RNVector.h
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VECTOR_H__
#define __RAYNE_VECTOR_H__

#include "RNBase.h"

namespace rn
{
    class vector2
    {
    public:
        vector2();
        vector2(const vector2& other);
        vector2(float n);
        vector2(float x, float y);
        
        bool operator== (const vector2 &other) const;
        bool operator!= (const vector2 &other) const;
        
        float x;
        float y;
    };
    
    RN_INLINE bool vector2::operator== (const vector2 &other) const
    {
        if(fabs(x - other.x) > kRNEpsilonFloat)
            return false;
        
        if(fabs(y - other.y) > kRNEpsilonFloat)
            return false;
        
        return true;
    }
    
    RN_INLINE bool vector2::operator!= (const vector2 &other) const
    {
        if(fabs(x - other.x) <= kRNEpsilonFloat)
            return false;
        
        if(fabs(y - other.y) <= kRNEpsilonFloat)
            return false;
        
        return true;
    }
}

#endif /* __RAYNE_VECTOR_H__ */
