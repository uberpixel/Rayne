import sys
import json
import os
import errno
import subprocess
import platform
import math
import shutil
import struct

def prepare():
    if platform.system() == 'Darwin':
        #Stop preparation if resizer file already exists
        resizerPath = os.path.dirname(sys.argv[0])
        resizerPath = os.path.join(resizerPath, 'ImageResizer/build/resizer')
        if os.path.isfile(resizerPath):
            return

        #macOS
        imgeResizerPath = os.path.dirname(sys.argv[0])
        imgeResizerPath = os.path.join(imgeResizerPath, 'ImageResizer/build')
        os.makedirs(imgeResizerPath)
        subprocess.call(['cmake', '..'], cwd=os.path.abspath(imgeResizerPath))
        subprocess.call(['make'], cwd=os.path.abspath(imgeResizerPath))

        astcencPath = os.path.dirname(sys.argv[0])
        subprocess.call(['make'], cwd=os.path.abspath(os.path.join(astcencPath, 'Vendor/astc-encoder/Source')))
        astcencPath = os.path.join(astcencPath, 'Vendor/astc-encoder/Source/astcenc-avx2')
        if not os.path.isfile(astcencPath):
            astcencPath = os.path.dirname(sys.argv[0])
            astcencPath = os.path.join(astcencPath, 'Vendor/astc-encoder/Source/astcenc-nointrin')
        subprocess.call(['chmod', '+x', astcencPath])

        bc7encPath = os.path.dirname(sys.argv[0])
        bc7encPath = os.path.join(bc7encPath, 'Vendor/bc7enc_rdo/build')
        os.makedirs(bc7encPath)
        subprocess.call(['cmake', '..'], cwd=os.path.abspath(bc7encPath))
        subprocess.call(['make'], cwd=os.path.abspath(bc7encPath))
        
    elif platform.system() == 'Windows':
        #Stop preparation if nvcompress file already exists
        nvTextureToolsExecutablePath = os.path.dirname(sys.argv[0])
        nvTextureToolsExecutablePath = os.path.join(nvTextureToolsExecutablePath, 'Vendor/nvidia-texture-tools/build/src/nvtt/tools/Release/nvcompress.exe')
        if os.path.isfile(nvTextureToolsExecutablePath):
            return

        #windows
        nvTextureToolsPath = os.path.dirname(sys.argv[0])
        nvTextureToolsPath = os.path.join(nvTextureToolsPath, 'Vendor/nvidia-texture-tools/build')
        os.makedirs(nvTextureToolsPath)
        subprocess.call(['cmake', '..'], cwd=os.path.abspath(nvTextureToolsPath))

        vswhereOutput = subprocess.check_output([os.path.join(os.environ["ProgramFiles(x86)"], "Microsoft Visual Studio/Installer/vswhere.exe"), "-latest", "-products", "*", "-requires", "Microsoft.Component.MSBuild"])
        vswhereLines = vswhereOutput.splitlines()
        for vswhereLine in vswhereLines:
            if vswhereLine.startswith("installationPath: "):
                vsInstallationPath = vswhereLine.split(": ", 1)[1]
        if not vsInstallationPath:
            print("No visual studio installation with MSBuild found!")
            return
        msbuildSearchDirectory = os.path.join(vsInstallationPath, "MSBuild")
        for root, dirs, files in os.walk(msbuildSearchDirectory):
            for file in files:
                if file == "MSBuild.exe":
                    msbuildPath = os.path.join(root, file)
        if not msbuildPath:
            print("MSBuild not found!")
            return
        subprocess.call([msbuildPath, 'NV.sln', '-target:nvcompress', '-maxcpucount', '/p:configuration=Release'], cwd=os.path.abspath(nvTextureToolsPath))
    elif platform.system() == 'Linux':
        #Stop preparation if nvcompress file already exists
        nvTextureToolsExecutablePath = os.path.dirname(sys.argv[0])
        nvTextureToolsExecutablePath = os.path.join(nvTextureToolsExecutablePath, 'Vendor/nvidia-texture-tools/build/src/nvtt/tools/nvcompress')
        if os.path.isfile(nvTextureToolsExecutablePath):
            return

        #linux
        #astcencPath = os.path.dirname(sys.argv[0])
        #astcencPath = os.path.join(astcencPath, 'Vendor/astc-encoder/Binary/linux-x64/astcenc')
        #subprocess.call(['chmod', '+x', astcencPath])

        nvTextureToolsPath = os.path.dirname(sys.argv[0])
        nvTextureToolsPath = os.path.join(nvTextureToolsPath, 'Vendor/nvidia-texture-tools/build')
        os.makedirs(nvTextureToolsPath)
        subprocess.call(['cmake', '..'], cwd=os.path.abspath(nvTextureToolsPath))
        subprocess.call(['make'], cwd=os.path.abspath(nvTextureToolsPath))


def needsToUpdateFile(sourceFile, targetFile):
    if os.path.isfile(sourceFile) and os.path.isfile(targetFile):
        if os.path.getmtime(targetFile) > os.path.getmtime(sourceFile):
            return False
    return True


def getPNGInfo(imageFile):
    print(imageFile)
    with open(imageFile, 'rb') as f:
        data = f.read()
        #if data[:8] == '\211PNG\r\n\032\n':# and (data[12:16] == 'IHDR'):
        w, h, b, t = struct.unpack('>LLbb', data[16:26])
        width = int(w)
        height = int(h)
        bitDepth = int(b)
        colorType = int(t)

        return width, height, bitDepth, colorType
        #else:
        #    raise Exception('not a png image')


def main():
    if len(sys.argv) < 2:
        print('python convert.py input.png [output (with optional extension for a specific format)]')
        return

    supportedFileExtensions = ['.png', '.dds', '.astc']
    requestedFileExtensions = supportedFileExtensions

    inputFileName, inputFileExtension = os.path.splitext(sys.argv[1])
    if inputFileExtension != '.png':
        print('Currently only png files are supported as input')
        return

    if len(sys.argv) >= 3:
        outputFileName, outputFileExtension = os.path.splitext(sys.argv[2])
        if outputFileExtension:
            if outputFileExtension in supportedFileExtensions:
                requestedFileExtensions = [outputFileExtension]
            else:
                print('Specified output files extension is not supported: ' + outputFileExtension + ' (needs to be one of ' + str(supportedFileExtensions) + ')')
                return
    else:
        outputFileName = inputFileName

    astcencPath = os.path.dirname(sys.argv[0])
    bc7encPath = os.path.dirname(sys.argv[0])
    imageResizerPath = os.path.dirname(sys.argv[0])
    if platform.system() == 'Darwin':
        astcencPath = os.path.join(astcencPath, 'Vendor/astc-encoder/Source/astcenc-avx2')
        if not os.path.isfile(astcencPath):
            astcencPath = os.path.join(os.path.dirname(sys.argv[0]), 'Vendor/astc-encoder/Source/astcenc-nointrin')
        bc7encPath = os.path.join(bc7encPath, 'Vendor/bc7enc_rdo/build/bc7enc')
        imageResizerPath = os.path.join(imageResizerPath, 'ImageResizer/build/resizer')
    elif platform.system() == 'Windows':
        astcencPath = os.path.join(astcencPath, 'Vendor/astc-encoder/Binary/windows-x64/astcenc.exe')
        nvTextureToolsPath = os.path.join(nvTextureToolsPath, 'Vendor/nvidia-texture-tools/build/src/nvtt/tools/Release/nvcompress.exe')
    elif platform.system() == 'Linux':
        astcencPath = os.path.join(astcencPath, 'Vendor/astc-encoder/Binary/linux-x64/astcenc.exe')
        nvTextureToolsPath = os.path.join(nvTextureToolsPath, 'Vendor/nvidia-texture-tools/build/src/nvtt/tools/nvcompress')
    else:
        print('Script needs to be updated with nvtexturetools path for platform: ' + platform.system())
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
            width, height, bitDepth, colorType = getPNGInfo(sourceFile)
            fullwidth = width;
            fullheight = height;
            bcFormat = 7
            if colorType == 0:
                bcFormat = 4
            elif colorType == 2:
                bcFormat = 1
            elif colorType == 4:
                bcFormat = 5
            elif colorType == 6:
                bcFormat = 7
            numLevels = int(1 + math.floor(math.log(max(width, height), 2)))
            bitsPerBlock = 16
            if bcFormat == 1 or bcFormat == 4:
                bitsPerBlock = 8
            topLevelCompressedSize = max(1, ((fullwidth + 3) / 4)) * bitsPerBlock #8 for dxt1, bc1 and bc4

            print('Number of mipmap levels: ' + str(numLevels))

            subprocess.call([imageResizerPath, sourceFile, outputFileName + inputFileExtension, str(numLevels), '-srgb'])

            for i in range(0, numLevels):
                callparams = [bc7encPath]
                if bcFormat == 1:
                    callparams.append('-1')
                elif bcFormat == 3:
                    callparams.append('-3')
                elif bcFormat == 4:
                    callparams.append('-4')
                elif bcFormat == 5:
                    callparams.append('-5')
                callparams.extend(['-f', '-g', outputFileName + '.' + str(i) + inputFileExtension, outputFileName + '.' + str(i) + '.dds'])
                subprocess.call(callparams)
                os.remove(outputFileName + '.' + str(i) + inputFileExtension)

            #Extract the dds format value from the highest level file
            ddsFormatValue = 99
            if bcFormat == 1:
                ddsFormatValue = 72
            elif bcFormat == 3:
                ddsFormatValue = 78
            elif bcFormat == 4:
                ddsFormatValue = 81
            elif bcFormat == 5:
                ddsFormatValue = 84
            #with open(outputFileName + '.0.dds', 'rb') as source:
            #    source.seek(128) #Skip first header and magic number
            #    ddsFormatValue = struct.unpack('I', source.read(4))[0]
                
            with open(targetFile, 'wb') as outputFile:
                #Write DDS header
                outputFile.write(struct.pack('I', 0x20534444)) #Magic number
                outputFile.write(struct.pack('I', 124)) #dwSize
                outputFile.write(struct.pack('I', 0x1|0x2|0x4|0x1000|0x20000|0x80000)) #dwFlags
                outputFile.write(struct.pack('I', fullheight)) #dwHeight
                outputFile.write(struct.pack('I', fullwidth)) #dwWidth
                outputFile.write(struct.pack('I', int(topLevelCompressedSize))) #dwPitchOrLinearSize
                outputFile.write(struct.pack('I', 0)) #dwDepth
                outputFile.write(struct.pack('I', numLevels)) #dwMipMapCount
                outputFile.write(struct.pack('11I', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)) #dwReserved1

                outputFile.write(struct.pack('I', 32)) #ddspf.dwSize
                outputFile.write(struct.pack('I', 0x4)) #ddspf.dwFlags
                outputFile.write(struct.pack('cccc', b'D', b'X', b'1', b'0')) #ddspf.dwFourCC
                outputFile.write(struct.pack('I', 0)) #ddspf.dwRGBBitCount
                outputFile.write(struct.pack('I', 0)) #ddspf.dwRBitMask
                outputFile.write(struct.pack('I', 0)) #ddspf.dwGBitMask
                outputFile.write(struct.pack('I', 0)) #ddspf.dwBBitMask
                outputFile.write(struct.pack('I', 0)) #ddspf.dwABitMask

                capsValue = 0x1000 #Is a texture
                if numLevels > 1:
                    capsValue |= 0x8 #Has multiple surfaces (like mipmaps)
                    capsValue |= 0x400000 #Has mipmaps

                outputFile.write(struct.pack('I', capsValue)) #dwCaps
                outputFile.write(struct.pack('I', 0)) #dwCaps2
                outputFile.write(struct.pack('I', 0)) #dwCaps3
                outputFile.write(struct.pack('I', 0)) #dwCaps4
                outputFile.write(struct.pack('I', 0)) #dwReserved2

                #Write DX10 header
                outputFile.write(struct.pack('I', ddsFormatValue)) #dxgiFormat BC7_UNORM_SRGB
                outputFile.write(struct.pack('I', 3)) #resourceDimension D3D10_RESOURCE_DIMENSION_TEXTURE2D
                outputFile.write(struct.pack('I', 0)) #miscFlag
                outputFile.write(struct.pack('I', 1)) #arraySize
                outputFile.write(struct.pack('I', 0x0)) #miscFlags2

                for i in range(0, numLevels):
                    tempSourceFile = outputFileName + '.' + str(i) + '.dds'
                    if os.path.isfile(tempSourceFile):
                        with open(tempSourceFile, 'rb') as source:
                            source.seek(148) #Skip headers and magic number
                            outputFile.write(source.read())
                        os.remove(tempSourceFile)

    if '.astc' in requestedFileExtensions:
        #Convert to ASTC format for mobile platforms
        sourceFile = inputFileName + inputFileExtension
        targetFile = outputFileName + '.astc'
        if needsToUpdateFile(sourceFile, targetFile):
            width, height = getImageSize(sourceFile)
            numLevels = int(1 + math.floor(math.log(max(width, height), 2)))

            print('Number of mipmap levels: ' + str(numLevels))

            subprocess.call([imageResizerPath, sourceFile, outputFileName + inputFileExtension, str(numLevels), '-srgb'])

            for i in range(0, numLevels):
                #subprocess.call(['sh', os.path.basename(compressonatorPath), '-fd', 'ASTC', sys.argv[1], sys.argv[2]], cwd=os.path.abspath(os.path.dirname(compressonatorPath)))
                subprocess.call([astcencPath, '-cs', outputFileName + '.' + str(i) + inputFileExtension, outputFileName + '.' + str(i) + '.astc', '6x6', '-fast'])
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
