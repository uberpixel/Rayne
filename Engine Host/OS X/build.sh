mkdir -p "$PROJECT_DIR/../../Build/"

rm -rf "$PROJECT_DIR/../../Build/$EXECUTABLE_NAME.app"
cp -r "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME.app" "$PROJECT_DIR/../../Build/"

cd "$PROJECT_DIR/../../Engine Test/Game"
xcrun xcodebuild -project Game-osx.xcodeproj -configuration $CONFIGURATION
rm -rf "$PROJECT_DIR/../../Engine Test/Game/build"