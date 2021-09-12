import sys
import os
import errno
import subprocess
import struct
import shutil
import json

def needsToUpdateFile(sourceFile, targetFile):
    if os.path.isfile(sourceFile) and os.path.isfile(targetFile):
        if os.path.getmtime(targetFile) > os.path.getmtime(sourceFile):
            return False
    return True

def getTextureSpec(resourceSpec, relpath, platform):
    textureSpec = dict()
    textureSpec["compression"] = "copy"

    relpath = os.path.normpath(relpath)
    relpath = relpath.replace("\\", "/")
    if "textures" in resourceSpec:
        platformCompressionName = "compression~"+platform

        if platformCompressionName in resourceSpec["textures"]:
            textureSpec["compression"] = resourceSpec["textures"][platformCompressionName].lower()
        elif "compression" in resourceSpec["textures"]:
            textureSpec["compression"] = resourceSpec["textures"]["compression"].lower()

        if "overrides" in resourceSpec["textures"]:
            overrides = resourceSpec["textures"]["overrides"]
            path = relpath
            while True:
                if path in overrides:
                    if platformCompressionName in overrides[path]:
                        textureSpec["compression"] = overrides[path][platformCompressionName].lower()
                    elif "compression" in overrides[path]:
                        textureSpec["compression"] = overrides[path]["compression"].lower()
                    break

                path = os.path.dirname(path)
                if len(path) == 0:
                    break

    textureSpec["extension"] = ".png"
    if textureSpec["compression"].startswith("astc"):
        textureSpec["extension"] = ".astc"
    elif textureSpec["compression"].startswith("bc"):
        textureSpec["extension"] = ".dds"
        
    textureSpec["parameters"] = ""
    if textureSpec["compression"].startswith("astc"):
        textureSpec["parameters"] = textureSpec["compression"][5:]

    return textureSpec

def needsToCopyFile(resourceSpec, relpath, platform, isDemo):
    relpath = os.path.normpath(relpath)
    relpath = relpath.replace("\\", "/")
    if "filters" in resourceSpec:
        filterDict = {}
        if isDemo:
            if "demo~"+platform in resourceSpec["filters"]:
                filterDict = resourceSpec["filters"]["demo~"+platform]
            elif "demo" in resourceSpec["filters"]:
                filterDict = resourceSpec["filters"]["demo"]
        else:
            if "game~"+platform in resourceSpec["filters"]:
                filterDict = resourceSpec["filters"]["game~"+platform]
            elif "game" in resourceSpec["filters"]:
                filterDict = resourceSpec["filters"]["game"]

        if "exclude" in filterDict:
            excludes = filterDict["exclude"]
            path = relpath
            while True:
                if path in excludes:
                    return False

                path = os.path.dirname(path)
                if len(path) == 0:
                    break

    return True

def getTexturesForModelFile(file):
    textures = list()
    with open(file, 'rb') as modelFile:
        magic = modelFile.read(4)
        magic = struct.unpack('<i', magic)[0]
        if magic != 352658064:
            print('Not an sgm file!')
            return textures

        version = modelFile.read(1)
        version = struct.unpack('<B', version)[0]
        if version != 3:
            print('Unsupported sgm file version (' + str(version) + ')!')
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
                    textureFilenameBytes = modelFile.read(textureFilenameLength)
                    textureFilename = struct.unpack('<'+str(textureFilenameLength)+'s', textureFilenameBytes)[0].decode('utf-8')
                    textures.append(textureFilename[:-1]) #remove 0 byte at the end

            colorCount = modelFile.read(1)
            colorCount = struct.unpack('<B', colorCount)[0]
            modelFile.seek(colorCount * (1 + 4*4), os.SEEK_CUR)

    return textures

def main():
    if len(sys.argv) < 4:
        print('python pack.py inputFolder outputFolder platform [--resourcespec=filename.json --skip-textures --is-demo]')
        return

    pythonExecutable = sys.executable
    scriptDirectory = os.path.dirname(sys.argv[0])
    sourceDirectory = sys.argv[1]
    targetDirectory = sys.argv[2]
    platform = None
    if sys.argv[3] in ['windows', 'macos', 'linux', 'android']:
        platform = sys.argv[3]

    if not platform:
        print("No valid platform specified (" + sys.argv[3] + ")!")
        return

    skipTextures = False
    isDemo = False
    resourceSpecFile = None
    for i in range(4, len(sys.argv), 1):
        if sys.argv[i] == '--skip-textures':
            skipTextures = True
        if sys.argv[i].startswith('--resourcespec='):
            resourceSpecFile = sys.argv[i][15:]
        if sys.argv[i] == '--is-demo':
            isDemo = True

    resourceSpec = dict()
    if resourceSpecFile:
        with open(resourceSpecFile, 'r') as jsonFile:
            resourceSpec = json.load(jsonFile)

    preferredTextureExtension = ''
    preferredShaderType = ''
    textureExtensionsToSkipForCompressed = ['.png']
    if platform:
        if platform == 'windows' or platform == 'macos' or platform == 'linux':
            preferredTextureExtension = '.dds'
            textureExtensionsToSkipForCompressed = ['.png', '.astc']
        elif platform == 'android':
            preferredTextureExtension = '.astc'
            textureExtensionsToSkipForCompressed = ['.png', '.dds']
        else:
            print('Platform not supported')

        if platform == 'windows':
            preferredShaderType = 'cso,spirv'
        elif platform == 'macos':
            preferredShaderType = 'metal'
        elif platform == 'linux' or platform == 'android':
            preferredShaderType = 'spirv'

    shaderConverter = os.path.join(scriptDirectory, '../ShaderProcessor/convert.py')
    textureConverter = os.path.join(scriptDirectory, '../TextureCompression/convert.py')

    globalFileToSkip = dict()

    #Compile shaders
    if resourceSpecFile and "shaders" in resourceSpec and "libraries" in resourceSpec["shaders"]:
        for shaderLibrary in resourceSpec["shaders"]["libraries"]:
            shaderLibraryPath = os.path.join(sourceDirectory, shaderLibrary)
            shaderOutputPath = os.path.join(targetDirectory, os.path.dirname(shaderLibrary))
            print(shaderOutputPath)
            callArray = [pythonExecutable, shaderConverter, shaderLibraryPath, preferredShaderType, shaderOutputPath, os.path.dirname(shaderLibrary)]
            subprocess.call(callArray)
            globalFileToSkip[shaderLibrary] = True

            #exclude shader files that are part of the library from getting copied on their own
            with open(shaderLibraryPath, 'r') as shaderLibraryData:
                shaderLibraryJson = json.load(shaderLibraryData)
                for shaderFile in shaderLibraryJson:
                    if 'file' in shaderFile:
                        globalFileToSkip[os.path.join(os.path.dirname(shaderLibrary), shaderFile['file'])] = True

    #loop through all subfolders and files
    for currentSourceDirectory, subdirs, files in os.walk(sourceDirectory):
        currentRelativePath = os.path.relpath(currentSourceDirectory, sourceDirectory)
        currentTargetDirectory = os.path.join(targetDirectory, currentRelativePath)
        if not os.path.exists(currentTargetDirectory):
            os.makedirs(currentTargetDirectory)

        filesToSkip = dict()

        #Convert and copy textures
        if not skipTextures:
            for filename in files:
                if resourceSpecFile:
                    if needsToCopyFile(resourceSpec, os.path.join(currentRelativePath, filename), platform, isDemo):
                        #Convert based on settings in resource spec file
                        textureFileName, textureFileExtension = os.path.splitext(filename)
                        if textureFileExtension == '.png':
                            textureSpec = getTextureSpec(resourceSpec, os.path.join(currentRelativePath, filename), platform)

                            textureInputPath = os.path.join(currentSourceDirectory, filename)
                            textureOutputPath = os.path.join(currentTargetDirectory, textureFileName + textureSpec["extension"])
                            callArray = [pythonExecutable, textureConverter, textureInputPath, textureOutputPath]
                            if len(textureSpec["parameters"]) > 0:
                                callArray.append(textureSpec["parameters"])
                            subprocess.call(callArray)
                            filesToSkip[filename] = True

                else:
                    #If no resource spec is specified fallback to only convert model textures
                    sourceFilePath = os.path.join(currentSourceDirectory, filename)
                    targetFilePath = os.path.join(currentTargetDirectory, filename)

                    if sourceFilePath.endswith(".sgm"):
                        textures = getTexturesForModelFile(sourceFilePath)
                        for texture in textures:
                            textureFileName, textureFileExtension = os.path.splitext(texture)
                            if textureFileExtension == '.*':
                                textureInputFilename = textureFileName + preferredTextureExtension
                                textureInputPath = os.path.join(currentSourceDirectory, textureInputFilename)
                                if os.path.isfile(textureInputPath):
                                    for extension in textureExtensionsToSkipForCompressed:
                                        textureInputFilename = textureFileName + extension
                                        filesToSkip[textureInputFilename] = True
                                    continue #If a file in the preferred format exists, use it instead of compressing the png file

                                textureInputFilename = textureFileName + '.png'
                                textureInputPath = os.path.join(currentSourceDirectory, textureInputFilename)
                                if os.path.isfile(textureInputPath):
                                    textureOutputPath = os.path.join(currentTargetDirectory, textureFileName + preferredTextureExtension)
                                    subprocess.call([pythonExecutable, textureConverter, textureInputPath, textureOutputPath])
                                    for extension in textureExtensionsToSkipForCompressed:
                                        textureInputFilename = textureFileName + extension
                                        filesToSkip[textureInputFilename] = True

        for filename in files:
            if not filename in filesToSkip and not os.path.join(currentRelativePath, filename) in globalFileToSkip:
                if skipTextures:
                    filebasename, fileextension = os.path.splitext(filename)
                    if fileextension in ['.dds', '.astc', '.png']:
                        continue

                if needsToCopyFile(resourceSpec, os.path.join(currentRelativePath, filename), platform, isDemo):
                    sourceFilePath = os.path.join(currentSourceDirectory, filename)
                    targetFilePath = os.path.join(currentTargetDirectory, filename)
                    if needsToUpdateFile(sourceFilePath, targetFilePath):
                        shutil.copy2(sourceFilePath, targetFilePath)


if __name__ == '__main__':
    main()
