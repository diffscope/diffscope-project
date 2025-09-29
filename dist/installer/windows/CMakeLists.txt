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
    setup.iss.in
    "${CMAKE_CURRENT_BINARY_DIR}/setup.iss"
    @ONLY
)

configure_file(
    setup-unofficial-languages.iss.in
    "${CMAKE_CURRENT_BINARY_DIR}/setup-unofficial-languages.iss"
    @ONLY
)

configure_file(
    uninstall.iss.in
    "${CMAKE_CURRENT_BINARY_DIR}/uninstall.iss"
    @ONLY
)

configure_file(
    uninstall-unofficial-languages.iss.in
    "${CMAKE_CURRENT_BINARY_DIR}/uninstall-unofficial-languages.iss"
    @ONLY
)
