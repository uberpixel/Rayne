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


def copyAndroidBuildSystem(fromdir, projectRoot, buildConfig):
	bundleID = getSettingFromConfig("android", "bundle-id", buildConfig).encode('utf-8')
	projectName = getSettingFromConfig("android", "name", buildConfig).encode('utf-8')
	cmakeTargets = ", ".join(getSettingFromConfig("android", "cmake-targets", buildConfig)).encode('utf-8')
	#cmakeVersion = subprocess.check_output(['cmake', '--version'])
	#cmakeVersion = cmakeVersion.splitlines()[0]
	#cmakeVersion = cmakeVersion.split(b" ")[2]
	cmakeVersion = b"3.18.1" #This one can be installed with android sdk manager!
	cmakeTargetsList = getSettingFromConfig("android", "cmake-targets", buildConfig)
	newCmakeTargetList = list()
	for target in cmakeTargetsList:
		newCmakeTargetList.append(("\""+target+"\"").encode('utf-8'))
	cmakeTargets = b", ".join(newCmakeTargetList)
	androidPermissions = getSettingFromConfig("android", "permissions", buildConfig)
	permissionsString = b""
	if androidPermissions:
		for permission in androidPermissions:
			permissionsString += b"    <uses-permission android:name=\"" + permission.encode('utf-8') + b"\" />\n";

	androidDependencies = getSettingFromConfig("android", "android-dependencies", buildConfig)
	dependenciesString = b""
	if androidDependencies:
		for dependency in androidDependencies:
			if dependency["type"] == "gradle":
				dependenciesString += b"	implementation '" + dependency["name"].encode('utf-8') + b"'\n";
			elif dependency["type"] == "aar":
				dependenciesString += b"	compile files('" + os.path.join(projectRoot, dependency["path"]).encode('utf-8') + b"')\n";

	androidActivity = "android.app.NativeActivity"
	customAndroidActivity = getSettingFromConfig("android", "android-custom-native-activity", buildConfig)
	if customAndroidActivity:
		androidActivity = customAndroidActivity.split("/")[-1]
		androidActivity = androidActivity.split(".")[0]
		androidActivity = "." + androidActivity

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
				fileContent = fileContent.replace(b"__RN_BUNDLE_ID__", bundleID)
				fileContent = fileContent.replace(b"__RN_PROJECT_NAME__", projectName)
				fileContent = fileContent.replace(b"__RN_ANDROID_ACTIVITY__", androidActivity.encode('utf-8'))
				fileContent = fileContent.replace(b"__RN_LIBRARY_NAME__", projectName.replace(b" ", b""))
				fileContent = fileContent.replace(b"__RN_CMAKE_VERSION__", cmakeVersion)
				fileContent = fileContent.replace(b"__RN_CMAKE_TARGETS__", cmakeTargets)
				fileContent = fileContent.replace(b"__RN_PERMISSIONS__", permissionsString)
				fileContent = fileContent.replace(b"__RN_ANDROID_DEPENDENCIES__", dependenciesString)

				with open(writeFilePath, 'wb') as writeFile:
					writeFile.write(fileContent)

	if customAndroidActivity:
		activityPath = os.path.join(projectRoot, customAndroidActivity)
		bundleIDComponents = bundleID.decode("utf-8").split('.')
		copyToFolder(activityPath, "app/src/main/java/" + "/".join(bundleIDComponents))

