import sys
import json
import os
import errno
import subprocess
import platform
import pathlib

def getNeedsUpdate(scriptFile, libraryFile, sourceFile, directory, pattern):
    referenceChangeTime = os.path.getmtime(scriptFile)
    libraryChangeTime = os.path.getmtime(libraryFile)
    if libraryChangeTime > referenceChangeTime:
        referenceChangeTime = libraryChangeTime
    sourceChangeTime = os.path.getmtime(sourceFile)
    if sourceChangeTime > referenceChangeTime:
        referenceChangeTime = sourceChangeTime
    pathlist = pathlib.Path(directory).glob(pattern)
    counter = 0
    for path in pathlist:
        counter += 1
        path_in_str = str(path)
        fileChangeTime = os.path.getmtime(path_in_str)
        if referenceChangeTime > fileChangeTime:
            return True
    return counter == 0

def removePermutations(directory, pattern):
    pathlist = pathlib.Path(directory).glob(pattern)
    for path in pathlist:
        path.unlink()

def main():
    if len(sys.argv) < 4:
        print('Specify shader json file followed by requested formats as comma separated list with no spaces (dxil,cso,spirv,metal_macos,metal_ios), output directory path [and optional resource folder relative path] as parameters')
        return

    with open(sys.argv[1], 'r') as sourceJsonData:
        sourceJson = json.load(sourceJsonData)

    if not sourceJson:
        print('No data found.')
        return

    pythonExecutable = sys.executable
    enableDebugSymbols = True

    outDirName = sys.argv[3]
    if not os.path.exists(outDirName):
        try:
            os.makedirs(outDirName)
        except OSError as exc: # Guard against race condition
            if exc.errno != errno.EEXIST:
                raise

    resourceRelativePath = None
    if len(sys.argv) >= 5:
        resourceRelativePath = sys.argv[4]
    if not resourceRelativePath:
        resourceRelativePath = ''

    jsonDirectory, jsonFileName = os.path.split(sys.argv[1])
    destinationJson = list()

    shaderConductorCmdPath = os.path.dirname(sys.argv[0])
    supportedFormats = ['dxil', 'cso', 'spirv', 'metal_macos', 'metal_ios']
    shaderConductorExectutableName = 'ShaderConductorCmd'
    if platform.system() == 'Darwin':
        supportedFormats = ['spirv', 'metal_macos', 'metal_ios']
    elif platform.system() == 'Windows':
        preprocessHLSLPath = os.path.join(shaderConductorCmdPath, 'preprocessForHLSL.py')
        shaderConductorExectutableName = 'ShaderConductorCmd.exe'
        fxcCmdPath = 'C:/Program Files (x86)/Windows Kits/10/bin/x64/fxc.exe'
    elif platform.system() == 'Linux':
        supportedFormats = ['spirv', 'metal_macos', 'metal_ios']
    else:
        print('Script needs to be updated with ShaderConductor path for platform: ' + platform.system())
        return

    shaderConductorSearchPath = pathlib.Path(os.path.join(shaderConductorCmdPath, 'Vendor/ShaderConductor/Build'))
    for path in shaderConductorSearchPath.glob('**/' + shaderConductorExectutableName):
        print(path)
        shaderConductorCmdPath = path
        break

    requestedFormats = sys.argv[2].split(',')
    outFormats = list()
    for request in requestedFormats:
        if request in supportedFormats:
            outFormats.append(request)

    hlslFile = False

    for shaderFile in sourceJson:
        if not 'file' in shaderFile or not 'shaders' in shaderFile:
            continue

        sourceFile = shaderFile['file']
        shaders = shaderFile['shaders']

        filePath, extension = os.path.splitext(sourceFile)
        filePath, fileName = os.path.split(filePath)
        sourceFile = os.path.join(jsonDirectory, sourceFile)

        if 'cso' in outFormats:
            hlslFile = os.path.join(outDirName, fileName + '.hlsl')
            subprocess.call([pythonExecutable, preprocessHLSLPath, sourceFile, hlslFile])

        for shader in shaders:
            if not 'name' in shader or not 'type' in shader:
                continue

            if shader['type'] == 'vertex':
                shaderType = 'vs'
            elif shader['type'] == 'fragment':
                shaderType = 'ps'
            elif shader['type'] == 'compute':
                shaderType = 'cs'

            shaderSignature = None
            shaderOptionsList = None
            shaderOptionsExcludesList = None
            shaderOptionsDependenciesList = None
            shaderOptionsDict = None
            if 'signature' in shader:
                shaderSignature = shader['signature']
                if 'options' in shaderSignature:
                    shaderOptions = shaderSignature['options']
                    if type(shaderOptions) is list:
                        shaderOptionsList = shaderOptions
                    elif type(shaderOptions) is dict:
                        shaderOptionsDict = shaderOptions
                        if "defines" in shaderOptionsDict:
                            shaderOptionsList = shaderOptionsDict["defines"]
                        if "excludes" in shaderOptionsDict:
                            shaderOptionsExcludesList = shaderOptionsDict["excludes"]
                        if "dependencies" in shaderOptionsDict:
                            shaderOptionsDependenciesList = shaderOptionsDict["dependencies"]

            entryName = shader['name']

            destinationShaderList = list()
            destinationShader = dict()
            destinationShader['type'] = shader['type']
            destinationShader['name'] = entryName
            if 'has_instancing' in shader:
                destinationShader["has_instancing"] = shader['has_instancing']
            if shaderSignature:
                destinationShader['signature'] = shaderSignature;
            destinationShaderList.append(destinationShader)

            destinationShaderFile = dict()
            destinationShaderFile['shaders'] = destinationShaderList

            permutations = list()
            if shaderOptionsList:
                permutationCount = 2**len(shaderOptionsList)
                for i in range(0, permutationCount):
                    permutation = dict()
                    permutation["parameters"] = list()
                    permutation["identifier"] = i
                    permutationOptions = list()
                    for n, option in enumerate(shaderOptionsList):
                        permutation["parameters"].append('-D')
                        permutationValue = '0'
                        if(i & (1 << n)) != 0:
                            permutationValue = '1'
                            permutationOptions.append(option)
                        permutation["parameters"].append(option + '=' + permutationValue)

                    isValidPermutation = True
                    if shaderOptionsExcludesList:
                        for exclude in shaderOptionsExcludesList:
                            isValidPermutation = False
                            for check in exclude:
                                if not check in permutationOptions:
                                    isValidPermutation = True
                                    break
                            if not isValidPermutation:
                                print("excluding permutation " + str(i) + ": " + str(permutationOptions))
                                break

                    if isValidPermutation and shaderOptionsDependenciesList:
                        for option in permutationOptions:
                            if option not in shaderOptionsDependenciesList:
                                continue
                            optionHasDependencies = False
                            while True:
                                option = shaderOptionsDependenciesList[option]
                                if option not in permutationOptions:
                                    isValidPermutation = False
                                    break
                                if option not in shaderOptionsDependenciesList:
                                    break

                            if not isValidPermutation:
                                print("excluding permutation " + str(i) + " because of missing dependencies: " + str(permutationOptions))
                                break
                    
                    if isValidPermutation: 
                        permutations.append(permutation)
            else:
                permutation = dict()
                permutation["parameters"] = list()
                permutation["identifier"] = 0
                permutations.append(permutation)

            skipShaderCompiling = False
            if not getNeedsUpdate(sys.argv[0], sys.argv[1], sourceFile, outDirName, fileName + "." + shaderType + ".*.*"):
                print("Shaders for file " + sourceFile + " are already up to date. Skipping.")
                skipShaderCompiling = True

            for outFormat in outFormats:
                outFileFormat = outFormat
                if outFormat == 'dxil':
                    compilerOutFormat = 'dxil'
                    destinationShaderFile['file~d3d12'] = resourceRelativePath + '/' + fileName + '.' + shaderType + '.' + outFormat
                elif outFormat == 'cso':
                    compilerOutFormat = 'cso'
                    destinationShaderFile['file~d3d12'] = resourceRelativePath + '/' + fileName + '.' + shaderType + '.' + outFormat
                elif outFormat == 'spirv':
                    compilerOutFormat = 'spirv'
                    destinationShaderFile['file~vulkan'] = resourceRelativePath + '/' + fileName + '.' + shaderType + '.' + outFormat
                elif outFormat == 'metal_macos' or outFormat == 'metal_ios':
                    outFileFormat = 'metal'
                    if outFormat == 'metal_macos':
                        compilerOutFormat = 'msl_macos'
                    else:
                        compilerOutFormat = 'msl_ios'

                    if platform.system() == 'Darwin':
                        destinationShaderFile['file~metal'] = resourceRelativePath + '/' + fileName + '.' + shaderType + '.metallib'
                    else:
                        destinationShaderFile['file~metal'] = resourceRelativePath + '/' + fileName + '.' + shaderType + '.metal'

                if not skipShaderCompiling:
                    if outFormat == 'metal_macos' or outFormat == 'metal_ios':
                        removePermutations(outDirName, fileName + "." + shaderType + ".*.metal")
                        removePermutations(outDirName, fileName + "." + shaderType + ".*.metallib")
                    else:
                        removePermutations(outDirName, fileName + "." + shaderType + ".*."+outFormat)

                for permutationDict in permutations:
                    permutation = permutationDict["parameters"]
                    permutationOutFile = os.path.join(outDirName, fileName + '.' + shaderType + '.' + str(permutationDict["identifier"]) + '.' + outFileFormat)

                    if outFormat == 'cso':
                        parameterList = [fxcCmdPath, '-I', '.', '-Fo', permutationOutFile, '-E', entryName, '-T', shaderType + '_5_1', hlslFile]
                    else:
                        parameterList = [shaderConductorCmdPath, '-I', sourceFile, '-O', permutationOutFile, '--minorshadermodel', '2', '-E', entryName, '-S', shaderType, '-T', compilerOutFormat]

                    if outFormat == 'dxil' or outFormat == 'cso':
                        parameterList.append("-DRN_RENDERER_D3D12=1")
                        permutation = [p.replace('RN_USE_MULTIVIEW', '__RN_USE_MULTIVIEW__') for p in permutation] #exclude multiview stuff for d3d12 without effecting the permutation index for now
                    elif outFormat == 'spirv':
                        if "has_16bit" in shader and shader["has_16bit"] == True:
                            parameterList.append("--16bittypes")
                            parameterList.append("true")

                        parameterList.append("-DRN_RENDERER_VULKAN=1")
                    elif outFormat == 'metal_macos' or outFormat == 'metal_ios':
                        if "has_16bit" in shader and shader["has_16bit"] == True:
                            parameterList.append("--16bittypes")
                            parameterList.append("true")
                        parameterList.append("-DRN_RENDERER_METAL=1")

                    parameterList.append("-DRN_SHADER_TYPE_" + shaderType.upper() + "=1") #Something about this is not working correctly...

                    if len(permutation) > 0:
                        parameterList.extend(permutation)

                    if not skipShaderCompiling:
                        print(parameterList)
                        subprocess.call(parameterList)

                        if (outFormat == 'metal_macos' or outFormat == 'metal_ios') and platform.system() == 'Darwin':
                            bitcodeOutFile = permutationOutFile + '.air'
                            libOutFile = os.path.join(outDirName, fileName + '.' + shaderType + '.' + str(permutationDict["identifier"]) + '.metallib')
                            if outFormat == 'metal_macos':
                                if enableDebugSymbols:
                                    subprocess.call(['xcrun', '-sdk', 'macosx', 'metal', '-gline-tables-only', '-MO', '-c', permutationOutFile, '-o', bitcodeOutFile])
                                else:
                                    subprocess.call(['xcrun', '-sdk', 'macosx', 'metal', '-c', permutationOutFile, '-o', bitcodeOutFile])
                                subprocess.call(['xcrun', '-sdk', 'macosx', 'metallib', bitcodeOutFile, '-o', libOutFile])
                            else:
                                if enableDebugSymbols:
                                    subprocess.call(['xcrun', '-sdk', 'iphoneos', 'metal', '-gline-tables-only', '-MO', '-c', permutationOutFile, '-o', bitcodeOutFile])
                                else:
                                    subprocess.call(['xcrun', '-sdk', 'iphoneos', 'metal', '-c', permutationOutFile, '-o', bitcodeOutFile])
                                subprocess.call(['xcrun', '-sdk', 'iphoneos', 'metallib', bitcodeOutFile, '-o', libOutFile])
                            os.remove(permutationOutFile)
                            os.remove(bitcodeOutFile)

            destinationJson.append(destinationShaderFile)

        if hlslFile:
            os.remove(hlslFile)

        with open(os.path.join(outDirName, 'Shaders.json'), 'w') as destinationJsonData:
            json.dump(destinationJson, destinationJsonData, indent=4, sort_keys=True)

if __name__ == '__main__':
    main()
