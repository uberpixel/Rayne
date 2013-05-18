#!/bin/bash

BASEDIR=$(dirname $0)

cd "$BASEDIR"

rm -rf ./Documentation/html
doxygen ./Documentation/Doxyfile
