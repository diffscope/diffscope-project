string(REPLACE "/" "\\" CMAKE_INSTALL_PREFIX_WIN "${CMAKE_INSTALL_PREFIX}")

if(DEFINED INNOSETUP_MESSAGE_FILES_DIR)
    string(REPLACE "/" "\\" INNOSETUP_MESSAGE_FILES_DIR_WIN "${INNOSETUP_MESSAGE_FILES_DIR}")
    set(INNOSETUP_SETUP_UNOFFICIAL_LANGUAGES "#include \"setup-unofficial-languages.iss\"")
    set(INNOSETUP_UNINSTALL_UNOFFICIAL_LANGUAGES "#include \"uninstall-unofficial-languages.iss\"")
else()
    set(INNOSETUP_SETUP_UNOFFICIAL_LANGUAGES "")
    set(INNOSETUP_UNINSTALL_UNOFFICIAL_LANGUAGES "")
endif()

if(NOT DEFINED APPLICATION_INSTALLER_OUTPUT_DIR)
    set(APPLICATION_INSTALLER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/dist")
endif()
string(REPLACE "/" "\\" APPLICATION_INSTALLER_OUTPUT_DIR_WIN "${APPLICATION_INSTALLER_OUTPUT_DIR}")

if(NOT DEFINED APPLICATION_INSTALLER_FILE_BASE)
    set(APPLICATION_INSTALLER_FILE_BASE "installer")
endif()

string(TIMESTAMP _current_year "%Y")

configure_file(
    "${CMAKE_SOURCE_DIR}/dist/installer/windows/setup.iss.in"
    "${CMAKE_BINARY_DIR}/dist/setup.iss"
    @ONLY
)

configure_file(
    "${CMAKE_SOURCE_DIR}/dist/installer/windows/setup-unofficial-languages.iss.in"
    "${CMAKE_BINARY_DIR}/dist/setup-unofficial-languages.iss"
    @ONLY
)

configure_file(
    "${CMAKE_SOURCE_DIR}/dist/installer/windows/uninstall.iss.in"
    "${CMAKE_BINARY_DIR}/dist/uninstall.iss"
    @ONLY
)

configure_file(
    "${CMAKE_SOURCE_DIR}/dist/installer/windows/uninstall-unofficial-languages.iss.in"
    "${CMAKE_BINARY_DIR}/dist/uninstall-unofficial-languages.iss"
    @ONLY
)
