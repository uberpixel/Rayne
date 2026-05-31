if [ -z "$3" ]
then
    rm -rf Builds\$1_%$2
	python3 ../../Rayne/Tools/BuildHelper/CreateBuildProject.py build-config.json $1 $2
else
	rm -rf Builds\$1_%$2_%$3
	python3 ../../Rayne/Tools/BuildHelper/CreateBuildProject.py build-config.json $1 $2 $3
fi
