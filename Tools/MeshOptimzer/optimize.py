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
    if platform.system() != 'Windows':
        #Stop preparation if resizer file already exists
        optimizerPath = os.path.dirname(sys.argv[0])
        optimizerPath = os.path.join(optimizerPath, 'Optimizer/build/optimizer')
        if os.path.isfile(optimizerPath):
            return

        #macOS
        optimizerPath = os.path.dirname(sys.argv[0])
        optimizerPath = os.path.join(optimizerPath, 'Optimizer/build')
        if not os.path.exists(optimizerPath):
            os.makedirs(optimizerPath)
        subprocess.call(['cmake', '..'], cwd=os.path.abspath(optimizerPath))
        subprocess.call(['make'], cwd=os.path.abspath(optimizerPath))
        
    else:
        #Stop preparation if resizer file already exists
        optimizerPath = os.path.dirname(sys.argv[0])
        optimizerPath = os.path.join(optimizerPath, 'Optimizer/build/Release/optimizer.exe')
        if os.path.isfile(optimizerPath):
            return

        vswhereOutput = subprocess.check_output([os.path.join(os.environ["ProgramFiles(x86)"], "Microsoft Visual Studio/Installer/vswhere.exe"), "-latest", "-requires", "Microsoft.Component.MSBuild", "-find", "MSBuild\\**\\Bin\\MSBuild.exe"])
        vswhereLines = vswhereOutput.splitlines()
        msbuildPath = None
        if len(vswhereLines) > 0:
            msbuildPath = vswhereLines[0].decode("utf-8").strip()
            print("found msbuild: " + msbuildPath)
        if not msbuildPath:
            print("No visual studio installation with MSBuild found!")
            return

        optimizerPath = os.path.dirname(sys.argv[0])
        optimizerPath = os.path.join(optimizerPath, 'Optimizer/build')
        if os.path.exists(optimizerPath):
            shutil.rmtree(optimizerPath)
        os.makedirs(optimizerPath)
        subprocess.call(['cmake', '..'], cwd=os.path.abspath(optimizerPath))
        subprocess.call([msbuildPath, 'Optimizer.sln', '/p:configuration=Release'], cwd=os.path.abspath(optimizerPath))


def needsToUpdateFile(sourceFile, targetFile):
    if os.path.isfile(sourceFile) and os.path.isfile(targetFile):
        if os.path.getmtime(targetFile) > os.path.getmtime(sourceFile):
            return False
    return True


def main():
    if len(sys.argv) < 2:
        print('python optimize.py input.sgm [output.sgm]')
        return

    supportedFileExtensions = ['.sgm']
    requestedFileExtensions = supportedFileExtensions

    inputFileName, inputFileExtension = os.path.splitext(sys.argv[1])
    if inputFileExtension != '.sgm':
        print('Currently only sgm files are supported as input')
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

    optimizerPath = os.path.dirname(sys.argv[0])
    optimizerPath = os.path.join(optimizerPath, 'Optimizer/build/optimizer')
    if platform.system() == 'Windows':
        optimizerPath += '.exe'

    if '.sgm' in requestedFileExtensions:
        sourceFile = inputFileName + inputFileExtension
        targetFile = outputFileName + '.sgm'
        if needsToUpdateFile(sourceFile, targetFile):
            subprocess.call([optimizerPath, sourceFile, targetFile])

if __name__ == '__main__':
    prepare()
    main()
