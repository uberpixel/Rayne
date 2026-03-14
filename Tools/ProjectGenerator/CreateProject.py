import os
import sys
import datetime

def main():
	template = input("Template name? ")

	# project info
	project_name = input("Project name? ")
	project_prefix = input("Project prefix? ")
	company_name = input("Company name? ")
	bundle_id = input("Bundle ID? ")

	project_target = bundle_id.split(".")[-1]
	bundle_domain = ".".join(bundle_id.split(".")[:-1])
	year = str(datetime.datetime.now().year)

	# template source
	from_dir = os.path.join(os.path.dirname(sys.argv[0]), "Templates")
	from_dir = os.path.join(from_dir, template)

	# traverse template
	for root, subdirs, files in os.walk(from_dir):
		relative_root = os.path.relpath(root, from_dir)

		# copy dirs
		for subdir in subdirs:
			os.makedirs(os.path.join(relative_root, subdir))

		# copy files
		for filename in files:
			if filename == ".DS_Store":
				continue

			read_file_path = os.path.join(root, filename)

			if filename == "gitattributes":
				filename = ".gitattributes"

			# replace filename variables
			filename = filename.replace("__TMP__", project_prefix)
			filename = filename.replace("__TMP_APPLICATION_TARGET__", project_target)

			write_file_path = os.path.join(relative_root, filename)

			# copy content
			with open(read_file_path, 'rb') as read_file:
				file_content = read_file.read()

				# replace content variables
				file_content = file_content.replace(b"__TMP__", project_prefix.encode())
				file_content = file_content.replace(b"__TMP_BUNDLE_ID__", bundle_id.encode())
				file_content = file_content.replace(b"__TMP_BUNDLE_DOMAIN__", bundle_domain.encode())
				file_content = file_content.replace(b"__TMP_APPLICATION_NAME__", project_name.encode())
				file_content = file_content.replace(b"__TMP_APPLICATION_TARGET__", project_target.encode())
				file_content = file_content.replace(b"__TMP_COMPANY__", company_name.encode())
				file_content = file_content.replace(b"__TMP_YEAR__", year.encode())

				with open(write_file_path, 'wb') as write_file:
					write_file.write(file_content)

if __name__ == '__main__':
	main()
