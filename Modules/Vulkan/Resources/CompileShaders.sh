#!/bin/bash

for file in VulkanGLSL/*
do
	name=$(echo ${file#VulkanGLSL/})
	echo $name
	/Users/slin/Documents/Dev/glslang-master/BUILD/StandAlone/glslangValidator -V100 -o ${name}.spv ${file}
done
