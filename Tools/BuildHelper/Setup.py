import os
import sys
import platform
import urllib
import zipfile
import json
import Utilities


def downloadAndExtractURL(downloadURL, directory):
	currentDirectory = os.path.abspath(os.path.dirname(sys.argv[0]))
	os.chdir(currentDirectory)

	directory = os.path.join(currentDirectory, directory);

	if platform.system() == 'Windows':
		directory = "\\\\?\\" + directory;

	if not os.path.exists(directory):
		os.makedirs(directory)

	filePath = os.path.join(directory, 'download.zip')
	urllib.urlretrieve(downloadURL, filePath)

	zippedFile = zipfile.ZipFile(filePath, 'r')
	zippedFile.extractall(directory)
	zippedFile.close()

	if os.path.exists(filePath):
		os.remove(filePath)

	os.chdir("..")


def main():
	if len(sys.argv) < 2:
		print 'Missing Argument!'
		print 'Correct Usage:'
		print 'python Setup.py build-config.json'
		return

	with open(sys.argv[1]) as json_file:
		buildConfigData = json.load(json_file)

	if not buildConfigData:
		print("Failed to open " + sys.argv[1] + "!")
		return

	projectRootPath = os.path.dirname(sys.argv[1])
	if not projectRootPath:
		projectRootPath = ""
	projectRootPath = os.path.abspath(projectRootPath)

	dependencies = Utilities.getSettingFromConfig("windows", "dependencies", buildConfigData)
	for key in dependencies:
		downloadAndExtractURL(dependencies[key], os.path.join(projectRootPath, key))

if __name__ == '__main__':
	main()
