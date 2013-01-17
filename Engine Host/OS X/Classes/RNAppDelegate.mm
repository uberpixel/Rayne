//
//  RNAppDelegate.mm
//  RayneApp
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#import "RNAppDelegate.h"

@implementation RNAppDelegate

- (void)runGameLoop:(NSTimer *)timer
{
	if(!kernel->Tick())
	{
		[timer invalidate];
	}
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	kernel = RN::Kernel::SharedInstance();
	world = new RN::World(kernel);
	
	[NSTimer scheduledTimerWithTimeInterval:0.0f target:self selector:@selector(runGameLoop:) userInfo:nil repeats:YES];
}

@end
