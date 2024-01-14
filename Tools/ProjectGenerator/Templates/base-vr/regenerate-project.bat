python generate_changes.py

rmdir /s /q Builds\%1_%2
python ../../Rayne/Tools/BuildHelper/CreateBuildProject.py build-config.json %1 %2
