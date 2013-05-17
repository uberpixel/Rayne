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

- (void)failWithException:(RN::ErrorException)e
{
	NSMutableString *description = [NSMutableString string];
	
	[description appendFormat:@"%s\n", e.Description().c_str()];
	[description appendFormat:@"%s\n\n", e.AdditionalDetails().c_str()];
	[description appendString:@"Callstack:\n"];
	
	const std::vector<std::pair<uintptr_t, std::string>>& callstack = e.CallStack();
	for(auto i=callstack.begin(); i!=callstack.end(); i++)
	{
		std::pair<uintptr_t, std::string> pair = *i;
		[description appendFormat:@"0x%8lx %s\n", pair.first, pair.second.c_str()];
	}
	
	
	NSAlert *alert = [[NSAlert alloc] init];
	[alert setMessageText:@"Failed to start Rayne"];
	[alert setInformativeText:description];
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
				[self failWithException:e];
				break;
		}
	}
}

@end
