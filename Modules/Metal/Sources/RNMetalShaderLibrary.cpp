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
	RNDefineMeta(MetalSpecializedShaderLibrary, Object)

	MetalSpecializedShaderLibrary::MetalSpecializedShaderLibrary(id<MTLDevice> device, const String *source, const ShaderOptions *options)
	{
		MTLCompileOptions *metalOptions = [[MTLCompileOptions alloc] init];

		if(options)
		{
			const Dictionary *defines = options->GetDefines();
			if(defines)
			{
				NSMutableDictionary *metalDefines = [[NSMutableDictionary alloc] init];

				defines->Enumerate<Object, Object>([&](Object *value, const Object *key, bool &stop) {

					if(key->IsKindOfClass(String::GetMetaClass()))
					{
						NSString *keyString = [[NSString alloc] initWithUTF8String:static_cast<const String *>(key)->GetUTF8String()];

						if(value->IsKindOfClass(Number::GetMetaClass()))
						{
							NSNumber *valueNumber = [[NSNumber alloc] initWithLongLong:static_cast<Number *>(value)->GetInt64Value()];
							[metalDefines setObject:valueNumber forKey:keyString];
							[valueNumber release];
						}
						else if(value->IsKindOfClass(String::GetMetaClass()))
						{
							NSString *valueString = [[NSString alloc] initWithUTF8String:static_cast<const String *>(value)->GetUTF8String()];
							[metalDefines setObject:valueString forKey:keyString];
							[valueString release];
						}

						[keyString release];
					}

				});

				[metalOptions setPreprocessorMacros:metalDefines];
				[metalDefines release];
			}
		}

		NSError *error = nil;
		id<MTLLibrary> library = [device newLibraryWithSource:[NSString stringWithUTF8String:source->GetUTF8String()] options:metalOptions error:&error];
		[metalOptions release];

		if(!library)
			throw ShaderCompilationException([[error localizedDescription] UTF8String]);


		_library = library;
	}

	MetalSpecializedShaderLibrary::~MetalSpecializedShaderLibrary()
	{
		id<MTLLibrary> lib = (id<MTLLibrary>)_library;
		[lib release];
	}

	Shader *MetalSpecializedShaderLibrary::GetShaderWithName(const String *name)
	{
		id<MTLLibrary> lib = (id<MTLLibrary>)_library;
		id<MTLFunction> shader = [lib newFunctionWithName:[NSString stringWithUTF8String:name->GetUTF8String()]];

		if(!shader)
			return nullptr;

		MetalShader *temp = new MetalShader(shader);
		return temp->Autorelease();
	}

/*	Array *MetalSpecializedShaderLibrary::GetShaderNames() const
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
	}*/

	RNDefineMeta(MetalShaderLibrary, ShaderLibrary)

	MetalShaderLibrary::MetalShaderLibrary(id<MTLDevice> device, const String *source) :
		_device(device), _source(source->Retain()), _specializedLibraries(new Dictionary())
	{

	}
	MetalShaderLibrary::~MetalShaderLibrary()
	{
		_specializedLibraries->Release();
	}

	Shader *MetalShaderLibrary::GetShaderWithName(const String *name, const ShaderOptions *options)
	{
		MetalSpecializedShaderLibrary *specializedLibrary = _specializedLibraries->GetObjectForKey<MetalSpecializedShaderLibrary>(options);

		if(!specializedLibrary)
		{
			specializedLibrary = new MetalSpecializedShaderLibrary(_device, _source, options);
			_specializedLibraries->SetObjectForKey(specializedLibrary, options);
		}

		return specializedLibrary->GetShaderWithName(name);
	}

}
