//
//  RNAppDelegate.mm
//  RayneApp
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
//  documentation files (the "Software"), to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
//  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
//  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
	[alert release];
	
	[NSApp terminate:self];
}

- (void)failWithException:(RN::Exception)e
{
	NSMutableString *description = [NSMutableString string];
	
	[description appendFormat:@"%s\n", e.Reason().c_str()];
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
	[alert release];
	
	[NSApp terminate:self];
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"ApplePersistenceIgnoreState"];
	
	try
	{
		NSString *name = [[[NSBundle mainBundle] localizedInfoDictionary] objectForKey:@"CFBundleDisplayName"];
		if(!name)
			name = [[NSRunningApplication currentApplication] localizedName];
		
		kernel = new RN::Kernel(std::string([name UTF8String]));
		[NSTimer scheduledTimerWithTimeInterval:0.0 target:self selector:@selector(runGameLoop:) userInfo:nil repeats:YES];
	}
	catch(RN::Exception e)
	{
		switch(e.ExceptionType())
		{
			/*case RNErrorError(RN::kErrorGroupSystem, RN::kSystemGroupGeneric, RN::kSystemCPUUnsupported):
				[self failWithError:@"Your CPU is unsupported! Your CPU needs to support at least SSE and SSE2"];
				break;
				
			case RNErrorError(RN::kErrorGroupGraphics, RN::kGraphicsFramebufferGenericError, RN::kGraphicsNoHardware):
				[self failWithError:@"Your Graphics Card is unsupported! You need a GPU with support for at least OpenGL 3.2"];
				break;*/
				
			default:
				[self failWithException:e];
				break;
		}
	}
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
	delete kernel;
}

@end
