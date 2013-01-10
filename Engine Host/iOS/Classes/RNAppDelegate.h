//
//  RNAppDelegate.h
//  Rayne Player
//
//  Created by Sidney Just on 03.01.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#import <UIKit/UIKit.h>
#include <RNKernel.h>
#include <RNWorld.h>

@interface RNAppDelegate : UIResponder <UIApplicationDelegate>
{
@private
	RN::Kernel *kernel;
	RN::World *world;
}

@end
