//
//  RNList.h
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LIST_H__
#define __RAYNE_LIST_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
    class List;
    class ListMember : public Object
    {
    friend class List;
    public:
        ListMember();
        
        void AddNext(ListMember *member);
        void AddPrev(ListMember *member);
        
        ListMember *Next() const { return _next; }
        ListMember *Prev() const { return _prev; }
        
    private:
        List *_list;
        ListMember *_next;
        ListMember *_prev;
    };
    
    class List : public Object
    {
    friend class ListMember;
    public:
        List();
        virtual ~List();
        
        void AddFront(ListMember *member);
        void AddBack(ListMember *member);
        
        void Remove(ListMember *member);
        
        ListMember *Head() const { return _head; }
        ListMember *Tail() const { return _tail; }
        
    private:
        ListMember *_head;
        ListMember *_tail;
    };
}

#endif /* __RAYNE_LIST_H__ */
