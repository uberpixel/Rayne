//
//  RNAppDelegate.mm
//  RayneApp
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#import "RNAppDelegate.h"

@implementation RNAppDelegate

- (void)runGameLoop
{
	kernel->Update(0.0f);
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	kernel = RN::Kernel::SharedInstance();
	world = new RN::World(kernel);
	
	[NSTimer scheduledTimerWithTimeInterval:1.0f/60.0f target:self selector:@selector(runGameLoop) userInfo:nil repeats:YES];
}

@end
