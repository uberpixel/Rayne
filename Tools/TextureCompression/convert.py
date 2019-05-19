import sys
import json
import os
import errno
import subprocess
import platform
import math
import shutil

def prepare():
    if platform.system() == 'Darwin':

        #Stop preparation if nvcompress file already exists
        nvTextureToolsExecutablePath = os.path.dirname(sys.argv[0])
        nvTextureToolsExecutablePath = os.path.join(nvTextureToolsExecutablePath, 'Vendor/nvidia-texture-tools/build/src/nvtt/tools/nvcompress')
        if os.path.isfile(nvTextureToolsExecutablePath):
            return

        #macOS
        astcencPath = os.path.dirname(sys.argv[0])
        astcencPath = os.path.join(astcencPath, 'Vendor/astc-encoder/Binary/mac-x64/astcenc')
        subprocess.call(['chmod', '+x', astcencPath])

        nvTextureToolsPath = os.path.dirname(sys.argv[0])
        nvTextureToolsPath = os.path.join(nvTextureToolsPath, 'Vendor/nvidia-texture-tools/build')
        os.makedirs(nvTextureToolsPath)
        subprocess.call(['cmake', '..'], cwd=os.path.abspath(nvTextureToolsPath))
        subprocess.call(['make'], cwd=os.path.abspath(nvTextureToolsPath))
        
    elif platform.system() == 'Windows':
        #windows
        print 'not implemented for windows yet'


def needsToUpdateFile(sourceFile, targetFile):
    if os.path.isfile(sourceFile) and os.path.isfile(targetFile):
        if os.path.getmtime(targetFile) > os.path.getmtime(sourceFile):
            return False
    return True


def main():
    if len(sys.argv) < 2:
        print 'python convert.py input.png [output (with option extension for a specific format)]'
        return

    supportedFileExtensions = ['.png', '.dds', '.astc']
    requestedFileExtensions = supportedFileExtensions

    inputFileName, inputFileExtension = os.path.splitext(sys.argv[1])
    if inputFileExtension != '.png':
        print 'Currently only png files are supported as input'
        return

    if len(sys.argv) >= 3:
        outputFileName, outputFileExtension = os.path.splitext(sys.argv[2])
        if outputFileExtension:
            if outputFileExtension in supportedFileExtensions:
                requestedFileExtensions = [outputFileExtension]
            else:
                print 'Specified output files extension is not supported: ' + outputFileExtension + ' (needs to be one of ' + str(supportedFileExtensions) + ')'
                return
    else:
        outputFileName = inputFileName

    astcencPath = os.path.dirname(sys.argv[0])
    nvTextureToolsPath = os.path.dirname(sys.argv[0])
    if platform.system() == 'Darwin':
        astcencPath = os.path.join(astcencPath, 'Vendor/astc-encoder/Binary/mac-x64/astcenc')
        #compressonatorPath = os.path.join(astcencPath, 'Vendor/CMP3/CompressonatorCLI.sh')
        nvTextureToolsPath = os.path.join(nvTextureToolsPath, 'Vendor/nvidia-texture-tools/build/src/nvtt/tools/nvcompress')
    elif platform.system() == 'Windows':
        astcencPath = os.path.join(astcencPath, 'Vendor/astc-encoder/Binary/windows-x64/astcenc.exe')
    else:
        print 'Script needs to be updated with ShaderConductor path for platform: ' + platform.system()
        return

    if '.png' in requestedFileExtensions:
        sourceFile = inputFileName + inputFileExtension
        targetFile = outputFileName + '.png'
        if sourceFile != targetFile:
            if needsToUpdateFile(sourceFile, targetFile):
                shutil.copy2(sourceFile, targetFile)

    if '.dds' in requestedFileExtensions:
        #Convert to BCn format in DDS container for desktop platforms
        sourceFile = inputFileName + inputFileExtension
        targetFile = outputFileName + '.dds'
        if needsToUpdateFile(sourceFile, targetFile):
            subprocess.call([nvTextureToolsPath, '-dds10', '-srgb', '-bc1', inputFileName + inputFileExtension, outputFileName + '.dds'])

    if '.astc' in requestedFileExtensions:
        #Convert to ASTC format for mobile platforms
        sourceFile = inputFileName + inputFileExtension
        targetFile = outputFileName + '.astc'
        if needsToUpdateFile(sourceFile, targetFile):
            width = subprocess.check_output(['identify', '-format', '%[fx:w]', sourceFile])
            height = subprocess.check_output(['identify', '-format', '%[fx:h]', sourceFile])
            bitdepth = subprocess.check_output(['identify', '-format', '%[fx:z]', sourceFile])
            width = int(width)
            height = int(height)
            bitdepth = int(bitdepth)
            numLevels = int(1 + math.floor(math.log(max(width, height), 2)))

            print 'Number of mipmap levels: ' + str(numLevels)

            subprocess.call(['convert', sourceFile, '-flip', '-define', 'png:bit-depth='+str(bitdepth), outputFileName + '.0' + inputFileExtension])
            for i in range(1, numLevels):
                subprocess.call(['convert', outputFileName + '.' + str(i-1) + inputFileExtension, '-scale', '50%', '-define', 'png:bit-depth='+str(bitdepth), outputFileName + '.' + str(i) + inputFileExtension])

            for i in range(0, numLevels):
                #subprocess.call(['sh', os.path.basename(compressonatorPath), '-fd', 'ASTC', sys.argv[1], sys.argv[2]], cwd=os.path.abspath(os.path.dirname(compressonatorPath)))
                subprocess.call([astcencPath, '-c', outputFileName + '.' + str(i) + inputFileExtension, outputFileName + '.' + str(i) + '.astc', '6x6', '-medium'])
                os.remove(outputFileName + '.' + str(i) + inputFileExtension)
                
            with open(targetFile, 'wb') as outputFile:
                for i in range(0, numLevels):
                    tempSourceFile = outputFileName + '.' + str(i) + '.astc'
                    if os.path.isfile(tempSourceFile):
                        with open(tempSourceFile, 'rb') as source:
                            outputFile.write(source.read())
                        os.remove(tempSourceFile)


if __name__ == '__main__':
    prepare()
    main()
