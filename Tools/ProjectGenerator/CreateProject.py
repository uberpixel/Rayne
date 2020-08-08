import os
import sys
import datetime

def main():
	templateName = input("Template name? ")
	projectNameString = input("Project name? ")
	projectName = projectNameString.encode()
	projectTarget = projectNameString.replace(" ", "").encode()
	prefixString = input("Project prefix? ")
	prefix = prefixString.encode()
	companyName = input("Company name? ").encode()
	bundleID = input("Bundle ID? ").encode()
	year = str(datetime.datetime.now().year).encode()

	fromdir = os.path.join(os.path.dirname(sys.argv[0]), "Templates")
	fromdir = os.path.join(fromdir, templateName)

	for root, subdirs, files in os.walk(fromdir):
		relativeRoot = os.path.relpath(root, fromdir)

		for subdir in subdirs:
			os.makedirs(os.path.join(relativeRoot, subdir))

		for filename in files:
			if filename == ".DS_Store":
				continue
			readFilePath = os.path.join(root, filename)

			if filename == "gitattributes":
				filename = ".gitattributes"
			filename = filename.replace("__TMP__", prefixString)
			writeFilePath = os.path.join(relativeRoot, filename)
			
			with open(readFilePath, 'rb') as readFile:
				fileContent = readFile.read()
				fileContent = fileContent.replace("__TMP__".encode(), prefix)
				fileContent = fileContent.replace("__TMP_BUNDLE_ID__".encode(), bundleID)
				fileContent = fileContent.replace("__TMP_APPLICATION_NAME__".encode(), projectName)
				fileContent = fileContent.replace("__TMP_APPLICATION_TARGET__".encode(), projectTarget)
				fileContent = fileContent.replace("__TMP_COMPANY__".encode(), companyName)
				fileContent = fileContent.replace("__TMP_YEAR__".encode(), year)

				with open(writeFilePath, 'wb') as writeFile:
					writeFile.write(fileContent)

if __name__ == '__main__':
	main()
