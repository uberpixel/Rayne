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
		delete kernel;
		
		[timer invalidate];
		[NSApp terminate:self];
	}
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"ApplePersistenceIgnoreState"];
	[NSTimer scheduledTimerWithTimeInterval:0.0 target:self selector:@selector(runGameLoop:) userInfo:nil repeats:YES];
	
	kernel = new RN::Kernel();
}

@end
