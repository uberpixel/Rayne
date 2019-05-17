import sys
import json
import os
import errno
import subprocess
import platform
import math
import shutil

def main():
    if len(sys.argv) < 3:
        print 'python convert.py input.png output.atsc [-mips]'

    astcencPath = os.path.dirname(sys.argv[0])
    if platform.system() == 'Darwin':
        astcencPath = os.path.join(astcencPath, 'Vendor/astc-encoder/Binary/mac-x64/astcenc')
        #astcencPath = os.path.join(astcencPath, 'Vendor/CMP3/CompressonatorCLI.sh')
    elif platform.system() == 'Windows':
        astcencPath = os.path.join(astcencPath, 'Vendor/astc-encoder/Binary/windows-x64/astcenc.exe')
    else:
        print 'Script needs to be updated with ShaderConductor path for platform: ' + platform.system()
        return

    width = subprocess.check_output(['identify', '-format', '%[fx:w]', sys.argv[1]])
    height = subprocess.check_output(['identify', '-format', '%[fx:h]', sys.argv[1]])

    width = int(width)
    height = int(height)
    numLevels = int(1 + math.floor(math.log(max(width, height), 2)))

    print 'Number of mipmap levels: ' + str(numLevels)

    inputFileName, inputFileExtension = os.path.splitext(sys.argv[1])
    outputFileName, outputFileExtension = os.path.splitext(sys.argv[2])

    shutil.copy2(sys.argv[1], inputFileName + '.0' + inputFileExtension)
    for i in range(1, numLevels):
        subprocess.call(['convert', inputFileName + '.' + str(i-1) + inputFileExtension, '-scale', '50%', inputFileName + '.' + str(i) + inputFileExtension])

    for i in range(0, numLevels):
        subprocess.call([astcencPath, '-c', inputFileName + '.' + str(i) + inputFileExtension, outputFileName + '.' + str(i) + outputFileExtension, '6x6', '-medium'])
        os.remove(inputFileName + '.' + str(i) + inputFileExtension)


    #subprocess.call(['sh', os.path.basename(astcencPath), '-fd', 'ASTC', sys.argv[1], sys.argv[2]], cwd=os.path.abspath(os.path.dirname(astcencPath)))

    with open(sys.argv[2], 'wb') as outputFile:
        for i in range(0, numLevels):
            tempSourceFile = outputFileName + '.' + str(i) + outputFileExtension
            if os.path.isfile(tempSourceFile):
                with open(tempSourceFile, 'rb') as source:
                    outputFile.write(source.read())
                os.remove(tempSourceFile)

if __name__ == '__main__':
    main()
