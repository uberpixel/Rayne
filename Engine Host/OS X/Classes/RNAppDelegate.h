//
//  RNAppDelegate.h
//  RayneApp
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#import <Cocoa/Cocoa.h>
#include <RNKernel.h>
#include <RNWorld.h>

@interface RNAppDelegate : NSObject <NSApplicationDelegate>
{
@private
	RN::Kernel *kernel;
	RN::World *world;
}

@end
