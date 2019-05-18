import sys
import os
import errno
import subprocess
import struct
import shutil

def getTexturesForModelFile(file):
    textures = list()
    with open(file, 'rb') as modelFile:
        magic = modelFile.read(4)
        magic = struct.unpack('<i', magic)[0]
        if magic != 352658064:
            print 'Not an sgm file!'
            return textures

        version = modelFile.read(1)
        version = struct.unpack('<B', version)[0]
        if version != 3:
            print 'Unsupported sgm file version (' + str(version) + ')!'
            return textures

        materialCount = modelFile.read(1)
        materialCount = struct.unpack('<B', materialCount)[0]
        if materialCount == 0:
            return textures

        for material in range(0, materialCount):
            materialID = modelFile.read(1)
            uvSetCount = modelFile.read(1)
            uvSetCount = struct.unpack('<B', uvSetCount)[0]
            for uvSet in range(0, uvSetCount):
                textureCount = modelFile.read(1)
                textureCount = struct.unpack('<B', textureCount)[0]
                for texture in range(0, textureCount):
                    textureType = modelFile.read(1)
                    textureType = struct.unpack('<B', textureType)[0]
                    textureFilenameLength = modelFile.read(2)
                    textureFilenameLength = struct.unpack('<H', textureFilenameLength)[0]
                    textureFilename = ''
                    for byte in range(0, textureFilenameLength):
                        textureFilenameByte = modelFile.read(1)
                        textureFilename += struct.unpack('<s', textureFilenameByte)[0]

                    textures.append(textureFilename[:-1]) #remove 0 byte at the end

            colorCount = modelFile.read(1)
            colorCount = struct.unpack('<B', colorCount)[0]
            modelFile.seek(colorCount * (1 + 4*4), os.SEEK_CUR)

    return textures

def needsToUpdateFile(sourceFile, targetFile):
    if os.path.isfile(sourceFile) and os.path.isfile(targetFile):
        if os.path.getmtime(targetFile) > os.path.getmtime(sourceFile):
            return False
    return True

def main():
    if len(sys.argv) < 3:
        print 'python pack.py inputFolder outputFolder [platform (either \'pc\' or \'mobile\')]'
        return

    scriptDirectory = os.path.dirname(sys.argv[0])
    sourceDirectory = sys.argv[1]
    targetDirectory = sys.argv[2]

    preferredTextureExtension = ''
    if len(sys.argv) > 3:
        if sys.argv[3] == 'pc':
            preferredTextureExtension = '.dds'
        elif sys.argv[3] == 'mobile':
            preferredTextureExtension = '.astc'
        else:
            print 'Platform not supported'

    textureConverter = os.path.join(scriptDirectory, '../TextureCompression/convert.py')

    #loop through all subfolders and files
    for currentSourceDirectory, subdirs, files in os.walk(sourceDirectory):
        currentTargetDirectory = os.path.relpath(currentSourceDirectory, sourceDirectory)
        currentTargetDirectory = os.path.join(targetDirectory, currentTargetDirectory)
        if not os.path.exists(currentTargetDirectory):
            os.makedirs(currentTargetDirectory)

        filesToSkip = dict()
        for filename in files:
            sourceFilePath = os.path.join(currentSourceDirectory, filename)
            targetFilePath = os.path.join(currentTargetDirectory, filename)

            if sourceFilePath.endswith(".sgm"):
                textures = getTexturesForModelFile(sourceFilePath)
                for texture in textures:
                    textureFileName, textureFileExtension = os.path.splitext(texture)
                    if textureFileExtension == '.*':
                        textureInputFilename = textureFileName + '.png'
                        textureInputPath = os.path.join(currentSourceDirectory, textureInputFilename)
                        if os.path.isfile(textureInputPath):
                            textureOutputPath = os.path.join(currentTargetDirectory, textureFileName + preferredTextureExtension)
                            subprocess.call(['python', textureConverter, textureInputPath, textureOutputPath])
                            filesToSkip[textureInputFilename] = True

        for filename in files:
            if not filename in filesToSkip:
                sourceFilePath = os.path.join(currentSourceDirectory, filename)
                targetFilePath = os.path.join(currentTargetDirectory, filename)
                if needsToUpdateFile(sourceFilePath, targetFilePath):
                    shutil.copy2(sourceFilePath, targetFilePath)


if __name__ == '__main__':
    main()
