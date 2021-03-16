import os
import sys
import platform
import contextlib
import urllib.request
import zipfile
import tarfile
import json
import Utilities


def downloadAndExtractURL(downloadURL, directory):
    oldWorkingDirectory = os.getcwd()
    currentDirectory = os.path.abspath(os.path.dirname(sys.argv[0]))
    os.chdir(currentDirectory)

    directory = os.path.join(currentDirectory, directory);

    if platform.system() == 'Windows':
        directory = "\\\\?\\" + directory;

    if not os.path.exists(directory):
        os.makedirs(directory)

    fileType = 'zip'
    if downloadURL.endswith('.tar.gz'):
        fileType = 'tar.gz'
    elif downloadURL.endswith('.tar'):
        fileType = 'tar'

    filePath = os.path.join(directory, 'download.'+fileType)

    with open(filePath, 'wb') as out_file:
        with contextlib.closing(urllib.request.urlopen(downloadURL)) as fp:
            block_size = 1024 * 8
            while True:
                block = fp.read(block_size)
                if not block:
                    break
                out_file.write(block)

    if fileType == 'zip':
        zippedFile = zipfile.ZipFile(filePath, 'r')
        zippedFile.extractall(directory)
        zippedFile.close()
    elif fileType == 'tar':
        tar = tarfile.open(filePath, "r:")
        tar.extractall(directory)
        tar.close()
    elif fileType == 'tar.gz':
        tar = tarfile.open(filePath, "r:gz")
        tar.extractall(directory)
        tar.close()

    if os.path.exists(filePath):
        os.remove(filePath)

    os.chdir(oldWorkingDirectory)


def main():
	if len(sys.argv) < 2:
		print('Missing Argument!')
		print('Correct Usage:')
		print('python Setup.py build-config.json')
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
