//
//  RNAppDelegate.mm
//  RayneApp
//
//  Copyright 2013 by Überpixel. All rights reserved.
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

- (void)failWithError:(NSString *)error
{
	NSAlert *alert = [[NSAlert alloc] init];
	[alert setMessageText:@"Failed to start Rayne"];
	[alert setInformativeText:error];
	[alert runModal];
	
	[NSApp terminate:self];
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"ApplePersistenceIgnoreState"];
	
	try
	{
		kernel = new RN::Kernel();
		[NSTimer scheduledTimerWithTimeInterval:0.0 target:self selector:@selector(runGameLoop:) userInfo:nil repeats:YES];
	}
	catch(RN::ErrorException e)
	{
		switch(e.Error())
		{
			case RNErrorError(RN::kErrorGroupSystem, RN::kSystemGroupGeneric, RN::kSystemCPUUnsupported):
				[self failWithError:@"Your CPU is unsupported! Your CPU needs to support at least SSE and SSE2"];
				break;
				
			case RNErrorError(RN::kErrorGroupGraphics, RN::kGraphicsFramebufferGenericError, RN::kGraphicsNoHardware):
				[self failWithError:@"Your Graphics Card is unsupported! You need a GPU with support for at least OpenGL 3.2"];
				break;
				
			default:
				[self failWithError:@"Unknown error"];
				break;
		}
	}
}

@end
