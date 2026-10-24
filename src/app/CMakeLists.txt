# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

# The icons of the application and of the project files, written at build time from the images in
# icons/<name>. The installer of Windows takes the icon of the project files from the resources of
# the executable, and the bundle of macOS from its resources. Linux uses the images directly.
set(_ico_args)
set(_icns_args)
set(_icons)

if(WIN32 OR APPLE)
    find_package(qiconmaker CONFIG REQUIRED)
    include(AddIcon)

    if(WIN32)
        set(_suffix .ico)
    else()
        set(_suffix .icns)
    endif()

    foreach(_name IN ITEMS app dspx)
        set(_icon ${CMAKE_CURRENT_BINARY_DIR}/icons/${_name}${_suffix})
        add_icon(${_icon} ${CMAKE_CURRENT_SOURCE_DIR}/icons/${_name})
        list(APPEND _icons ${_icon})
    endforeach()

    if(WIN32)
        set(_ico_args ICO ${_icons})
    else()
        set(_icns_args ICNS ${CMAKE_CURRENT_BINARY_DIR}/icons/app.icns)
    endif()
endif()

ck_configure_application(
    ${_ico_args}
    ${_icns_args}
    INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/info.plist.in
)

if(_icons)
    target_sources(${PROJECT_NAME} PRIVATE ${_icons})
endif()

if(APPLE)
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/icons/dspx.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
endif()

qm_configure_target(${PROJECT_NAME}
    SOURCES main.cpp
    QT_LINKS_PRIVATE
        Core Widgets Qml Quick QuickControls2
    LINKS_PRIVATE
        ChorusKit::Loader
        ChorusKit::AppCore
        qjsonsettings
        loadapi
        uishell
    INCLUDE_PRIVATE
        ${CK_BUILD_INCLUDE_DIR}
)

if(APPLICATION_ENABLE_BREAKPAD)
    find_package(qBreakpad REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE qBreakpad::qBreakpad)
endif()

# Set configration file variables
set(APP_CONFIG_SPLASH_IMAGE "splash.png")

# Configure configuration file
set(_config_file ${CMAKE_CURRENT_BINARY_DIR}/config.json)
configure_file(config.json.in ${_config_file} @ONLY)

# Deploy install phase files
ck_add_shared_files(
    SKIP_BUILD
    SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSE DEST ${CK_BUILD_DOC_DIR}                    # license index
    SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSES/ DEST ${CK_BUILD_DOC_DIR}/LICENSES          # license texts
    SRC conf/${CK_PLATFORM_LOWER}/qt.conf DEST ${CK_BUILD_QT_CONF_DIR}                         # qt.conf
)

# Deploy build & install phase files
ck_add_shared_files(
    SRC ${_config_file} ${APP_CONFIG_SPLASH_IMAGE} DEST ${CK_BUILD_DATA_DIR}    # config.json
    # SRC conf/${CK_PLATFORM_LOWER}/qtmediate.json DEST ${CK_BUILD_QT_CONF_DIR}   # qtmediate.json
)

file(GLOB _icon_images icons/*)

qt_add_resources(${PROJECT_NAME} icons
    PREFIX /diffscope/icons
    BASE icons
    FILES ${_icon_images}
)

if(LINUX)
    add_subdirectory(share/linux)
endif()
