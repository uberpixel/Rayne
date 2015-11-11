//
//  RNMetalShaderLibrary.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#import <Metal/Metal.h>
#include "RNMetalShaderLibrary.h"
#include "RNMetalShader.h"

namespace RN
{
	RNDefineMeta(MetalShaderLibrary, ShaderLibrary)

	MetalShaderLibrary::MetalShaderLibrary(void *library) :
		_library(library)
	{
		id<MTLLibrary> lib = (id<MTLLibrary>)library;
		[lib retain];
	}
	MetalShaderLibrary::~MetalShaderLibrary()
	{
		id<MTLLibrary> lib = (id<MTLLibrary>)_library;
		[lib release];
	}



	Shader *MetalShaderLibrary::GetShaderWithName(const String *name)
	{
		id<MTLLibrary> lib = (id<MTLLibrary>)_library;
		id<MTLFunction> shader = [lib newFunctionWithName:[NSString stringWithUTF8String:name->GetUTF8String()]];

		if(!shader)
			return nullptr;

		MetalShader *temp = new MetalShader(shader);
		return temp->Autorelease();
	}
	Array *MetalShaderLibrary::GetShaderNames() const
	{
		Array *result = new Array();
		id<MTLLibrary> library = (id<MTLLibrary>)_library;

		NSArray *names = [library functionNames];
		NSUInteger count = [names count];

		for(NSUInteger i = 0; i < count; i ++)
		{
			NSString *name = [names objectAtIndex:i];

			result->AddObject(RNSTR([name UTF8String]));
		}

		return result->Autorelease();
	}
}
