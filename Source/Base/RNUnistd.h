//
//  RNUnistd.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//
#include "RNBaseInternal.h"

#if RN_PLATFORM_POSIX
	#include <unistd.h>
#elif RN_PLATFORM_WINDOWS

	// Check with http://stackoverflow.com/questions/341817/is-there-a-replacement-for-unistd-h-for-windows-visual-c
	 
	#include <stdlib.h>
	#include <fcntl.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <io.h>
	#include <stdio.h>
	//#include <getopt.h>	// Not needed
	#include <process.h>	// getpid(), exec stuff
	#include <direct.h>		// _getcwd(), _chdir()

	#define srandom srand
	#define random rand

	#define R_OK	4
	#define W_OK	2
	//#define X_OK	1	// Unsupported on windows
	#define F_OK	0

	#define access _access
	#define dup2 _dup2
	#define execve _execve
	#define ftruncate _chsize
	#define unlink _unlink
	#define fileno _fileno
	#define getcwd _getcwd
	#define chdir _chdir
	#define isatty _isatty
	#define lseek _lseek
		
	#define ssize_t signed long long	// Long long because size_t is defined as unsigned long long in MSVC

	#define STDIN_FILENO 0
	#define STDOUT_FILENO 1
	#define STDERR_FILENO 2

#endif