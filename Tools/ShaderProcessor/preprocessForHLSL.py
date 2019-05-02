import sys
import json
import os
import errno
import subprocess
import platform

def main():
    if len(sys.argv) != 3:
        print 'Specify input and output file'

    with open(sys.argv[1], 'r') as sourceShaderData:
        sourceString = sourceShaderData.read()

    if not sourceString:
        print 'Error reading file (' + sys.argv[1] + ')'
        return

    destinationString = ''

    currentPosition = 0
    nextPosition = sourceString.find('[[vk::', currentPosition)
    while nextPosition != -1:
        destinationString += sourceString[currentPosition:nextPosition]
        endPosition = sourceString.find(']]', nextPosition + 1)
        if endPosition != -1:
            currentPosition = endPosition + 2
        else:
            currentPosition = nextPosition + 1

        nextPosition = sourceString.find('[[vk::', currentPosition)

    destinationString += sourceString[currentPosition:]

    with open(sys.argv[2], 'w') as destinationShaderData:
        destinationShaderData.write(destinationString)

if __name__ == '__main__':
    main()
