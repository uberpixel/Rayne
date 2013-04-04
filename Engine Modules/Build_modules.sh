#!/bin/bash
BASEDIR="$(cd -P "$(dirname "$0")" && pwd)"
FILES="rayne-*"

cd "$BASEDIR"

for f in $FILES
do
    MODPATH="$BASEDIR/$f"
    cd "$MODPATH"
    
    xcrun xcodebuild -configuration "Release"
done