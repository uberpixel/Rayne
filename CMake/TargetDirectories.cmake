
macro(target_directories _TARGET)

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

    set("${TARGET}_INSTALL_LIB_DIR" lib CACHE PATH "Installation directory for libraries")
    set("${TARGET}_INSTALL_BIN_DIR" bin CACHE PATH "Installation directory for executables")
    set("${TARGET}_INSTALL_INCLUDE_DIR" include CACHE PATH "Installation directory for header files")

endmacro()
