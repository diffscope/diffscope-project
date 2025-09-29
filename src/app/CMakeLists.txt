add_subdirectory(icons)

set(_ico_args)
set(_icns_args)

if(WIN32)
    list(APPEND _ico_path
        ${CMAKE_CURRENT_BINARY_DIR}/icons/app.ico
        ${CMAKE_CURRENT_BINARY_DIR}/icons/dspx.ico
    )
    set(_filtered_ico_path "")
    foreach(_file IN LISTS _ico_path)
        if(EXISTS ${_file})
            list(APPEND _filtered_ico_path ${_file})
        endif()
    endforeach()
    set(_ico_args ICO ${_filtered_ico_path})
endif()

if(APPLE)
    set(_icns_path ${CMAKE_CURRENT_BINARY_DIR}/icons/app.icns)
    if(EXISTS ${_icns_path})
        set(_icns_args ICNS ${_icns_path})
    endif()
endif()



ck_configure_application(
    ${_ico_args}
    ${_icns_args}
)

qm_configure_target(${PROJECT_NAME}
    SOURCES main.cpp
    QT_LINKS_PRIVATE
        Core Widgets Qml
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
    SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSE DEST ${CK_BUILD_DOC_DIR}  # doc
    SRC conf/${CK_PLATFORM_LOWER}/qt.conf DEST ${CK_BUILD_QT_CONF_DIR}      # qt.conf
)

# Deploy build & install phase files
ck_add_shared_files(
    SRC ${_config_file} ${APP_CONFIG_SPLASH_IMAGE} DEST ${CK_BUILD_DATA_DIR}    # config.json
    # SRC conf/${CK_PLATFORM_LOWER}/qtmediate.json DEST ${CK_BUILD_QT_CONF_DIR}   # qtmediate.json
)

file(GLOB _icons icons/*)
list(FILTER _icons EXCLUDE REGEX CMakeLists.txt)

qt_add_resources(${PROJECT_NAME} icons
    PREFIX /diffscope/icons
    BASE icons
    FILES ${_icons}
)

if(WIN32)
    # Deploy windows icons
    ck_add_shared_files(
        SKIP_BUILD
        SRC share/win-icons/ DEST ${CK_BUILD_DATA_DIR}/icons
    )
elseif(APPLE)
# TODO...
else()
    # Add linux files
    ck_add_shared_files(
        SKIP_BUILD
        SRC share/applications share/icons DEST ${CK_BUILD_DATA_DIR}
    )
endif()