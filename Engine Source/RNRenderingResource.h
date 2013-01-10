//
//  RNRenderingResource.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RENDERERINGRESOURCE_H__
#define __RAYNE_RENDERERINGRESOURCE_H__

#include "RNBase.h"

namespace RN
{
	class RenderingResource
	{
	public:
		RenderingResource(const char *name)
		{
			SetName(name);
		}
		
		virtual void SetName(const char *name)
		{
			_name = (name != 0) ? name : "";
			_length = strlen(_name) + 1;
		}
		
		void Push();
		void Pop();
		
	protected:
		const char *_name;
		size_t _length;
	};
	
	RN_INLINE void RenderingResource::Push()
	{
#if GL_EXT_debug_marker
		glPushGroupMarkerEXT(_length, _name);
#endif
	}
	
	RN_INLINE void RenderingResource::Pop()
	{
#if GL_EXT_debug_marker
		glPopGroupMarkerEXT();
#endif
	}
}

#endif /* __RAYNE_RENDERERINGRESOURCE_H__ */
