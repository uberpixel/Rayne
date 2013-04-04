MODULES_PATH="$PROJECT_DIR/../../Engine Modules/"
"$MODULES_PATH/Build_modules.sh"

rm -rf "$PROJECT_DIR/Resources/modules"
cp -r "$MODULES_PATH/Build" "$PROJECT_DIR/Resources/modules"