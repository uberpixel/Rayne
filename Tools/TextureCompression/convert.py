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
        print 'python convert.py input.png'

    inputFileName, inputFileExtension = os.path.splitext(sys.argv[1])

    astcencPath = os.path.dirname(sys.argv[0])
    nvTextureToolsPath = os.path.dirname(sys.argv[0])
    if platform.system() == 'Darwin':
        astcencPath = os.path.join(astcencPath, 'Vendor/astc-encoder/Binary/mac-x64/astcenc')
        #astcencPath = os.path.join(astcencPath, 'Vendor/CMP3/CompressonatorCLI.sh')
        nvTextureToolsPath = os.path.join(nvTextureToolsPath, 'Vendor/nvidia-texture-tools/build/src/nvtt/tools/nvcompress')
    elif platform.system() == 'Windows':
        astcencPath = os.path.join(astcencPath, 'Vendor/astc-encoder/Binary/windows-x64/astcenc.exe')
    else:
        print 'Script needs to be updated with ShaderConductor path for platform: ' + platform.system()
        return

    #Convert to BCn format in DDS container for desktop platforms
    subprocess.call([nvTextureToolsPath, '-dds10', '-srgb', '-bc1', inputFileName + inputFileExtension, inputFileName + '.dds'])

    #Convert to ASTC format for desktop platforms
    width = subprocess.check_output(['identify', '-format', '%[fx:w]', sys.argv[1]])
    height = subprocess.check_output(['identify', '-format', '%[fx:h]', sys.argv[1]])

    width = int(width)
    height = int(height)
    numLevels = int(1 + math.floor(math.log(max(width, height), 2)))

    print 'Number of mipmap levels: ' + str(numLevels)

    shutil.copy2(sys.argv[1], inputFileName + '.0' + inputFileExtension)
    for i in range(1, numLevels):
        subprocess.call(['convert', inputFileName + '.' + str(i-1) + inputFileExtension, '-scale', '50%', inputFileName + '.' + str(i) + inputFileExtension])

    for i in range(0, numLevels):
        subprocess.call([astcencPath, '-c', inputFileName + '.' + str(i) + inputFileExtension, inputFileName + '.' + str(i) + '.astc', '6x6', '-medium'])
        os.remove(inputFileName + '.' + str(i) + inputFileExtension)


    #subprocess.call(['sh', os.path.basename(astcencPath), '-fd', 'ASTC', sys.argv[1], sys.argv[2]], cwd=os.path.abspath(os.path.dirname(astcencPath)))

    with open(inputFileName + '.astc', 'wb') as outputFile:
        for i in range(0, numLevels):
            tempSourceFile = inputFileName + '.' + str(i) + '.astc'
            if os.path.isfile(tempSourceFile):
                with open(tempSourceFile, 'rb') as source:
                    outputFile.write(source.read())
                os.remove(tempSourceFile)

if __name__ == '__main__':
    main()
