#!/usr/bin/python

import os
import platform
import logging
import json
import subprocess
import sys

logging.basicConfig()

try:
	basedir      = os.path.dirname(os.path.realpath(__file__))
	dependencies = json.load(open(os.path.join(basedir, 'dependencies.json')))
except IOError:
	print 'Couldn\'t load dependencies.json!'
	sys.exit(-1)


## OS X
def brew_install(dependency):
	print 'Installing {0}'.format(dependency['name'])

	command = '/usr/local/bin/brew install {0}'.format(dependency['formular'])
	pipe = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	(out, error) = pipe.communicate()
	pipe.wait()

	if os.path.isfile(dependency['path']) == False:
		print 'Failed to install {0}. Error:\n{1}'.format(dependency['name'], error)
		sys.exit(-1)

def install_osx():
	print 'Checking dependencies for OS X...'

	for dependency in dependencies['osx']:
		if os.path.isfile(dependency['path']) == False:
			brew_install(dependency)

	print 'Everything is good to go!'

## Fallback
def install_fail():
	print 'Unknown system ({0}), no idea how to install dependencies!'.format(platform.system())
	sys.exit(-1)

if __name__ == '__main__':

	{
		'Darwin': install_osx,
	}.get(platform.system(), install_fail)()

