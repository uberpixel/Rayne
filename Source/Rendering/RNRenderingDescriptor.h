//
//  RNRenderingDescriptor.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_RENDERINGDESCRIPTOR_H__
#define __RAYNE_RENDERINGDESCRIPTOR_H__

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNString.h"

namespace RN
{
	class Renderer;
	class RenderingDescriptor
	{
	public:
		RNAPI ~RenderingDescriptor();

		RNAPI virtual Renderer *CreateAndSetActiveRenderer() = 0;

		const String *GetIdentifier() const { return _identifier; }
		const String *GetAPI() const { return _api; }

	protected:
		RNAPI RenderingDescriptor(const String *identifier, const String *api);

	private:
		String *_identifier;
		String *_api;
	};
}


#endif /* __RAYNE_RENDERINGDESCRIPTOR_H__ */
