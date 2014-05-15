#Basic project setup
cmake_minimum_required(VERSION 2.6)
project(RayneAll)
#TODO: maybe add version here?

#Add vendor dependencies
add_subdirectory("Vendor")

#Add source directory for Rayne
add_subdirectory("Engine Source")

#Add Engine Test project
add_subdirectory("Engine Test")
