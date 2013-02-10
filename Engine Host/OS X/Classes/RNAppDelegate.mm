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
		[NSApp terminate:self];
	}
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"ApplePersistenceIgnoreState"];
	
	kernel = new RN::Kernel("libGame");	
	[NSTimer scheduledTimerWithTimeInterval:1.0f/60.0f target:self selector:@selector(runGameLoop:) userInfo:nil repeats:YES];
}

@end
