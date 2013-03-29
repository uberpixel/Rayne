//
//  RNAppDelegate.h
//  RayneApp
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#import <Cocoa/Cocoa.h>
#include <RNKernel.h>
#include <RNWorld.h>

@interface RNAppDelegate : NSObject <NSApplicationDelegate>
{
@private
	RN::Kernel *kernel;
}

@end
