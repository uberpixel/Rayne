#!/usr/bin/python

import os
import sys
import subprocess
import signal
import resource

UMASK = 0
WORKDIR = "/"

basedir = os.path.dirname(os.path.realpath(__file__))

path = '{0}/bot.py'.format(basedir)
command = [path]
command.extend(sys.argv)

if (hasattr(os, "devnull")):
	REDIRECT_TO = os.devnull
else:
	REDIRECT_TO = "/dev/null"

def CreateDaemon():
	pid = os.fork()

	if pid == 0:
		os.setsid()
		signal.signal(signal.SIGHUP, signal.SIG_IGN)

		pid = os.fork()

		if pid == 0:
			os.chdir(WORKDIR)
			os.umask(UMASK)
		else:
			os._exit(0)
	else:
		os._exit(0)

	maxfd = resource.getrlimit(resource.RLIMIT_NOFILE)[1]
	if maxfd == resource.RLIM_INFINITY:
		maxfd = 1024

	for fd in range(0, maxfd):
		try:
			os.close(fd)
		except OSError:	# ERROR, fd wasn't open to begin with (ignored)
			pass

	os.open(REDIRECT_TO, os.O_RDWR)
	os.dup2(0, 1)
	os.dup2(0, 2)

	return 0

if __name__ == "__main__":
	CreateDaemon()
	subprocess.Popen(command, shell=False, stdin=None, stdout=None, stderr=None, close_fds=True)
	
