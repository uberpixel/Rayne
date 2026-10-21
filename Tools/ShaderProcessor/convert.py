import sys
import json
import os
import errno
import subprocess
import platform
import pathlib
import concurrent.futures
import re

METAL_FORMATS = ('metal_macos', 'metal_ios', 'metal_ios_sim', 'metal_visionos', 'metal_visionos_sim')


def running_under_xcode():
    xcode_env_keys = (
        'XCODE_VERSION_ACTUAL',
        'XCODE_PRODUCT_BUILD_VERSION',
        'ACTION',
        'TARGET_BUILD_DIR',
        'OBJROOT',
    )
    return any(key in os.environ for key in xcode_env_keys)

def build_metal_commands(sdk, permutation_out_file, bitcode_out_file, lib_out_file, enable_debug_symbols):
    commands = list()
    metal_cmd = ['xcrun', '-sdk', sdk, 'metal']
    if enable_debug_symbols:
        metal_cmd.extend(['-gline-tables-only', '-MO'])
    metal_cmd.extend(['-c', permutation_out_file, '-o', bitcode_out_file])
    commands.append(metal_cmd)
    commands.append(['xcrun', '-sdk', sdk, 'metallib', bitcode_out_file, '-o', lib_out_file])
    return commands

def execute_command_sequence(commands, cleanup_paths, expected_outputs, metal_dispatch_base_patch_file):
    try:
        for index, command in enumerate(commands):
            print(command)
            result = subprocess.run(command)
            if result.returncode != 0:
                raise RuntimeError(f"Command failed with exit code {result.returncode}: {command}")

            if index == 0 and metal_dispatch_base_patch_file:
                print('Patch Metal compute dispatch base offsets')
                patch_metal_compute_dispatch_base(metal_dispatch_base_patch_file)

        for output in expected_outputs:
            if not os.path.isfile(output) or os.path.getsize(output) == 0:
                raise RuntimeError(f"Expected output missing or empty after compilation: {output}")

        for path in cleanup_paths:
            try:
                os.remove(path)
            except FileNotFoundError:
                pass
    except Exception:
        for output in expected_outputs:
            try:
                os.remove(output)
            except FileNotFoundError:
                pass
            except OSError as error:
                print(f"Failed to remove output {output}: {error}", file=sys.stderr)
        raise

def getNeedsUpdate(scriptFile, libraryFile, sourceFile, expectedOutputs):
    referenceChangeTime = os.path.getmtime(scriptFile)
    libraryChangeTime = os.path.getmtime(libraryFile)
    if libraryChangeTime > referenceChangeTime:
        referenceChangeTime = libraryChangeTime
    sourceChangeTime = os.path.getmtime(sourceFile)
    if sourceChangeTime > referenceChangeTime:
        referenceChangeTime = sourceChangeTime
    for output in expectedOutputs:
        if not os.path.isfile(output) or os.path.getsize(output) == 0:
            return True
        if referenceChangeTime > os.path.getmtime(output):
            return True
    return False

def removePermutations(directory, pattern):
    pathlist = pathlib.Path(directory).glob(pattern)
    for path in pathlist:
        path.unlink()

def patch_metal_compute_dispatch_base(permutation_out_file):
    dispatch_offsets_argument_name = 'rn_DispatchOffsets'
    max_buffer_index = 30
    buffer_index_pattern = re.compile(r'\[\[\s*buffer\s*\(\s*([0-9]+)\s*\)\s*\]\]')
    kernel_pattern = re.compile(r'(kernel\s+void\s+\w+\s*\()(?P<arguments>.*?)(\)\s*\{)', re.MULTILINE | re.DOTALL)

    with open(permutation_out_file, 'r') as file:
        source = file.read()

    if dispatch_offsets_argument_name in source:
        return

    existing_buffer_indices = [int(index) for index in buffer_index_pattern.findall(source)]
    dispatch_offsets_buffer_index = (max(existing_buffer_indices) + 1) if existing_buffer_indices else 0
    if dispatch_offsets_buffer_index > max_buffer_index:
        raise RuntimeError(f"Metal compute shader '{permutation_out_file}' has no free buffer slot for dispatch base offsets")

    def get_builtin_argument_name(arguments, builtin_name):
        match = re.search(r'\b(\w+)\s*\[\[\s*' + re.escape(builtin_name) + r'\s*\]\]', arguments)
        return match.group(1) if match else None

    def patch_kernel(match):
        arguments = match.group('arguments')
        thread_position_name = get_builtin_argument_name(arguments, 'thread_position_in_grid')
        threadgroup_position_name = get_builtin_argument_name(arguments, 'threadgroup_position_in_grid')
        if not thread_position_name and not threadgroup_position_name:
            return match.group(0)

        patched_arguments = arguments.rstrip() + f', constant uint4* {dispatch_offsets_argument_name} [[buffer({dispatch_offsets_buffer_index})]]'
        offset_lines = []
        if thread_position_name:
            offset_lines.append(f'    {thread_position_name} += {dispatch_offsets_argument_name}[0].xyz;')
        if threadgroup_position_name:
            offset_lines.append(f'    {threadgroup_position_name} += {dispatch_offsets_argument_name}[1].xyz;')

        return match.group(1) + patched_arguments + match.group(3) + '\n' + '\n'.join(offset_lines)

    patched_source = kernel_pattern.sub(patch_kernel, source)
    if patched_source == source:
        return

    with open(permutation_out_file, 'w') as file:
        file.write(patched_source)

def get_shader_type_short(shaderTypeName):
    if shaderTypeName == 'vertex':
        return 'vs'
    elif shaderTypeName == 'fragment':
        return 'ps'
    elif shaderTypeName == 'compute':
        return 'cs'

    return None

def get_compute_threads_per_group(source_file, entry_name):
    with open(source_file, 'r') as file:
        source = file.read()

    pattern = re.compile(r'\[\s*numthreads\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*\][^{;]*\b' + re.escape(entry_name) + r'\s*\(', re.MULTILINE | re.DOTALL)
    match = pattern.search(source)
    if not match:
        return None

    return [int(match.group(1)), int(match.group(2)), int(match.group(3))]

def main():
    if len(sys.argv) < 4:
        print('Specify shader json file followed by requested formats as comma separated list with no spaces (spirv,metal_macos,metal_ios,metal_ios_sim,metal_visionos,metal_visionos_sim), output directory path [and optional resource folder relative path] as parameters')
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
    supportedFormats = ['spirv', 'metal_macos', 'metal_ios', 'metal_ios_sim', 'metal_visionos', 'metal_visionos_sim']
    shaderConductorExectutableName = 'ShaderConductorCmd'
    if platform.system() == 'Darwin':
        supportedFormats = ['spirv', 'metal_macos', 'metal_ios', 'metal_ios_sim', 'metal_visionos', 'metal_visionos_sim']
    elif platform.system() == 'Windows':
        shaderConductorExectutableName = 'ShaderConductorCmd.exe'
    elif platform.system() == 'Linux':
        supportedFormats = ['spirv', 'metal_macos', 'metal_ios', 'metal_ios_sim', 'metal_visionos', 'metal_visionos_sim']
    else:
        print('Script needs to be updated with ShaderConductor path for platform: ' + platform.system())
        return

    shaderConductorSearchPath = pathlib.Path(os.path.join(shaderConductorCmdPath, 'Vendor/ShaderConductor/Build'))
    for path in shaderConductorSearchPath.glob('**/' + shaderConductorExectutableName):
        print(path)
        shaderConductorCmdPath = path
        break

    metal_sdk_map = {
        'metal_macos': 'macosx',
        'metal_ios': 'iphoneos',
        'metal_ios_sim': 'iphonesimulator',
        'metal_visionos': 'xros',
        'metal_visionos_sim': 'xrsimulator',
    }

    requestedFormats = sys.argv[2].split(',')
    outFormats = list()
    for request in requestedFormats:
        if request in supportedFormats:
            outFormats.append(request)

    command_jobs = list()

    hlslFile = False

    for shaderFile in sourceJson:
        if not 'file' in shaderFile or not 'shaders' in shaderFile:
            continue

        sourceFile = shaderFile['file']
        shaders = shaderFile['shaders']

        filePath, extension = os.path.splitext(sourceFile)
        filePath, fileName = os.path.split(filePath)
        sourceFile = os.path.join(jsonDirectory, sourceFile)

        shaderTypeCounts = dict()
        for shader in shaders:
            if not 'type' in shader:
                continue

            shaderType = get_shader_type_short(shader['type'])
            if shaderType:
                shaderTypeCounts[shaderType] = shaderTypeCounts.get(shaderType, 0) + 1

        for shader in shaders:
            if not 'name' in shader or not 'type' in shader:
                continue

            shaderType = get_shader_type_short(shader['type'])
            if not shaderType:
                continue

            entryName = shader['name']
            outputFileName = fileName + '.' + shaderType
            if shaderTypeCounts[shaderType] > 1:
                outputFileName = fileName + '.' + entryName + '.' + shaderType

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

            destinationShaderList = list()
            destinationShader = dict()
            destinationShader['type'] = shader['type']
            destinationShader['name'] = entryName
            if 'has_instancing' in shader:
                destinationShader["has_instancing"] = shader['has_instancing']
            if shader['type'] == 'compute':
                if 'threads_per_group' in shader:
                    destinationShader['threads_per_group'] = shader['threads_per_group']
                else:
                    threadsPerGroup = get_compute_threads_per_group(sourceFile, entryName)
                    if threadsPerGroup:
                        destinationShader['threads_per_group'] = threadsPerGroup
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
                                #print("excluding permutation " + str(i) + ": " + str(permutationOptions))
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
                                #print("excluding permutation " + str(i) + " because of missing dependencies: " + str(permutationOptions))
                                break
                    
                    if isValidPermutation: 
                        permutations.append(permutation)
            else:
                permutation = dict()
                permutation["parameters"] = list()
                permutation["identifier"] = 0
                permutations.append(permutation)

            expectedOutputs = list()
            for outFormat in outFormats:
                extension = outFormat
                if outFormat in METAL_FORMATS:
                    extension = 'metallib' if platform.system() == 'Darwin' else 'metal'
                for permutation in permutations:
                    expectedOutputs.append(os.path.join(outDirName, outputFileName + '.' + str(permutation["identifier"]) + '.' + extension))

            skipShaderCompiling = False
            if not getNeedsUpdate(sys.argv[0], sys.argv[1], sourceFile, expectedOutputs):
                print("Shaders for file " + sourceFile + " are already up to date. Skipping.")
                skipShaderCompiling = True

            for outFormat in outFormats:
                outFileFormat = outFormat
                if outFormat == 'spirv':
                    compilerOutFormat = 'spirv'
                    destinationShaderFile['file~vulkan'] = resourceRelativePath + '/' + outputFileName + '.' + outFormat
                elif outFormat in METAL_FORMATS:
                    outFileFormat = 'metal'
                    if outFormat == 'metal_macos':
                        compilerOutFormat = 'msl_macos'
                    else:
                        compilerOutFormat = 'msl_ios' #Should also work for the simulator and visionos

                    if platform.system() == 'Darwin':
                        destinationShaderFile['file~metal'] = resourceRelativePath + '/' + outputFileName + '.metallib'
                    else:
                        destinationShaderFile['file~metal'] = resourceRelativePath + '/' + outputFileName + '.metal'

                if not skipShaderCompiling:
                    if outFormat in METAL_FORMATS:
                        removePermutations(outDirName, outputFileName + ".*.metal")
                        removePermutations(outDirName, outputFileName + ".*.metallib")
                    else:
                        removePermutations(outDirName, outputFileName + ".*."+outFormat)

                for permutationDict in permutations:
                    permutation = permutationDict["parameters"]
                    permutationOutFile = os.path.join(outDirName, outputFileName + '.' + str(permutationDict["identifier"]) + '.' + outFileFormat)

                    parameterList = [shaderConductorCmdPath, '-I', sourceFile, '-O', permutationOutFile, '--minorshadermodel', '2', '-E', entryName, '-S', shaderType, '-T', compilerOutFormat]

                    if outFormat == 'spirv':
                        if "has_16bit" in shader and shader["has_16bit"] == True:
                            parameterList.append("--16bittypes")
                            parameterList.append("true")

                        parameterList.append("-DRN_RENDERER_VULKAN=1")
                    elif outFormat in METAL_FORMATS:
                        if "has_16bit" in shader and shader["has_16bit"] == True:
                            parameterList.append("--16bittypes")
                            parameterList.append("true")
                        parameterList.append("-DRN_RENDERER_METAL=1")

                    parameterList.append("-DRN_SHADER_TYPE_" + shaderType.upper() + "=1") #Something about this is not working correctly...

                    if len(permutation) > 0:
                        parameterList.extend(permutation)

                    if not skipShaderCompiling:
                        job_commands = [parameterList]
                        job_cleanup = list()
                        expected_output = permutationOutFile
                        metalDispatchBasePatchFile = None

                        if outFormat in metal_sdk_map and platform.system() == 'Darwin':
                            bitcodeOutFile = permutationOutFile + '.air'
                            libOutFile = os.path.join(outDirName, outputFileName + '.' + str(permutationDict["identifier"]) + '.metallib')
                            sdk = metal_sdk_map[outFormat]
                            if shaderType == 'cs':
                                metalDispatchBasePatchFile = permutationOutFile
                            metal_commands = build_metal_commands(sdk, permutationOutFile, bitcodeOutFile, libOutFile, enableDebugSymbols)
                            job_commands.extend(metal_commands)
                            job_cleanup.extend([permutationOutFile, bitcodeOutFile])
                            expected_output = libOutFile
                        elif outFormat in METAL_FORMATS and shaderType == 'cs':
                            metalDispatchBasePatchFile = permutationOutFile

                        command_jobs.append((job_commands, job_cleanup, [expected_output], metalDispatchBasePatchFile))

            destinationJson.append(destinationShaderFile)

        if hlslFile:
            os.remove(hlslFile)

    cpu_workers = os.cpu_count() or 1
    if running_under_xcode():
        cpu_workers = min(cpu_workers, 1)

    with concurrent.futures.ThreadPoolExecutor(max_workers=cpu_workers) as executor:
        futures = [executor.submit(execute_command_sequence, commands, cleanup_paths, outputs, metalDispatchBasePatchFile) for commands, cleanup_paths, outputs, metalDispatchBasePatchFile in command_jobs]
        for future in concurrent.futures.as_completed(futures):
            future.result()

    destinationJsonPath = os.path.join(outDirName, 'Shaders.json')
    with open(destinationJsonPath + '.tmp', 'w') as destinationJsonData:
        json.dump(destinationJson, destinationJsonData, indent=4, sort_keys=True)
    os.replace(destinationJsonPath + '.tmp', destinationJsonPath)

if __name__ == '__main__':
    main()
