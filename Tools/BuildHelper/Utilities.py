import os
import sys
import platform
import subprocess
import shutil

def setGradleProperty(file, key, value):
	oldData = None
	with open(file, "r") as f:
		oldData = f.read()

	lines = oldData.splitlines()
	properties = list()

	didOverwriteProperty = 0
	for line in lines:
		keyValue = line.split('=', 1)
		if len(keyValue) > 1:
			if keyValue[0] == key:
				keyValue[1] = value
				didOverwriteProperty = 1
		properties.append(keyValue)

	if didOverwriteProperty == 0:
		keyValue = list()
		keyValue.append(key)
		keyValue.append(value)
		properties.append(keyValue)

	with open(file, "w") as f:
		for keyValue in properties:
			if len(keyValue) > 1:
				f.write(keyValue[0]+'='+keyValue[1]+'\n')
			else:
				f.write(keyValue[0]+'\n')


def getBuildNumber(file):
	oldData = None
	with open(file, "r") as f:
		oldData = f.read()
	lines = oldData.splitlines()
	if len(lines) < 1:
		return 0
	return int(lines[0])


def getVersion(file):
	oldData = None
	with open(file, "r") as f:
		oldData = f.read()
	lines = oldData.splitlines()
	if len(lines) < 2:
		return '0.0.0'
	return lines[1]


def setVersionAndBuildNumber(file, version, buildNumber):
	lines = list()
	lines.append(str(buildNumber))
	lines.append(version)

	with open(file, "w") as f:
		for line in lines:
			f.write(line+'\n')


def copyToFolder(file, folder):
	if not os.path.isdir(folder):
		os.makedirs(folder)
	shutil.copy2(file, folder)


def getSettingFromConfig(platform, setting, config, platformoverride=True):
	value = None
	if setting in config:
		value = config[setting]

	platformSetting = setting+"~"+platform
	if platformSetting in config:
		if platformoverride:
			value = config[platformSetting]
		else:
			value += " "+config[platformSetting]

	return value


def copyAndroidBuildSystem(fromdir, todir, buildConfig):
	bundleID = getSettingFromConfig("android", "bundle-id", buildConfig)
	projectName = getSettingFromConfig("android", "name", buildConfig)
	cmakeTargets = ", ".join(getSettingFromConfig("android", "cmake-targets", buildConfig))
	cmakeVersion = subprocess.check_output(['cmake', '--version'])
	cmakeVersion = cmakeVersion.splitlines()[0]
	cmakeVersion = cmakeVersion.split(" ")[2]
	cmakeTargetsList = getSettingFromConfig("android", "cmake-targets", buildConfig)
	newCmakeTargetList = list()
	for target in cmakeTargetsList:
		newCmakeTargetList.append("\""+target+"\"")
	cmakeTargets = ", ".join(newCmakeTargetList)
	androidPermissions = getSettingFromConfig("android", "permissions", buildConfig)
	permissionsString = ""
	for permission in androidPermissions:
		permissionsString += "    <uses-permission android:name=\"" + permission + "\" />\n";

	for root, subdirs, files in os.walk(fromdir):
		relativeRoot = os.path.relpath(root, fromdir)

		for subdir in subdirs:
			os.makedirs(os.path.join(relativeRoot, subdir))

		for filename in files:
			if filename == ".DS_Store":
				continue
			readFilePath = os.path.join(root, filename)
			writeFilePath = os.path.join(relativeRoot, filename)
			
			with open(readFilePath, 'rb') as readFile:
				fileContent = readFile.read()
				fileContent = fileContent.replace("__RN_BUNDLE_ID__", bundleID)
				fileContent = fileContent.replace("__RN_PROJECT_NAME__", projectName)
				fileContent = fileContent.replace("__RN_CMAKE_VERSION__", cmakeVersion)
				fileContent = fileContent.replace("__RN_CMAKE_TARGETS__", cmakeTargets)
				fileContent = fileContent.replace("__RN_PERMISSIONS__", permissionsString)

				with open(writeFilePath, 'wb') as writeFile:
					writeFile.write(fileContent)

