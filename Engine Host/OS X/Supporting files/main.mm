//
//  main.mm
//  Rayne Player
//
//  Created by Sidney Just on 12.03.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#include <Rayne.h>

int main(int argc, char *argv[])
{
	int result = 0;
	
	try
	{
		RN::ParseCommandLine(argc, argv);
		result = NSApplicationMain(argc, (const char **)argv);
	}
	catch(RN::ErrorException e)
	{
		RN::__HandleException(e);
	}
	
	return result;
}
