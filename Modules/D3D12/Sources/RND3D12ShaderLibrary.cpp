//
//  RND3D12ShaderLibrary.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12ShaderLibrary.h"
#include "RND3D12Shader.h"

namespace RN
{
	RNDefineMeta(D3D12ShaderLibrary, ShaderLibrary)

	D3D12ShaderLibrary::D3D12ShaderLibrary(void *library) :
		_library(library)
	{
/*		id<MTLLibrary> lib = (id<MTLLibrary>)library;
		[lib retain];*/
	}
	D3D12ShaderLibrary::~D3D12ShaderLibrary()
	{
/*		id<MTLLibrary> lib = (id<MTLLibrary>)_library;
		[lib release];*/
	}



	Shader *D3D12ShaderLibrary::GetShaderWithName(const String *name)
	{
/*		id<MTLLibrary> lib = (id<MTLLibrary>)_library;
		id<MTLFunction> shader = [lib newFunctionWithName:[NSString stringWithUTF8String:name->GetUTF8String()]];

		if(!shader)
			return nullptr;
			*/
		
		D3D12Shader *temp = new D3D12Shader(nullptr);
		return temp->Autorelease();
	}
	Array *D3D12ShaderLibrary::GetShaderNames() const
	{
		Array *result = new Array();
/*		id<MTLLibrary> library = (id<MTLLibrary>)_library;

		NSArray *names = [library functionNames];
		NSUInteger count = [names count];

		for(NSUInteger i = 0; i < count; i ++)
		{
			NSString *name = [names objectAtIndex:i];

			result->AddObject(RNSTR([name UTF8String]));
		}*/

		return result->Autorelease();
	}
}
