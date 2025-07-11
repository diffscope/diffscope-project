add_subdirectory(libs/3rdparty)

# Add `ChorusKitAPI`
if(NOT TARGET ChorusKit::Loader)
    find_package(ChorusKit CONFIG REQUIRED)
endif()

include(${CK_CMAKE_MODULES_DIR}/ChorusKitAPI.cmake)

# ----------------------------------
# Project Variables
# ----------------------------------
set(CK_APPLICATION_NAME ${PROJECT_NAME})
set(CK_APPLICATION_DESCRIPTION ${PROJECT_NAME})
set(CK_APPLICATION_VERSION ${PROJECT_VERSION})
set(CK_APPLICATION_VENDOR "Team OpenVPI")
set(CK_DEV_START_YEAR 2019)

set(CK_BUILDINFO_PREFIX APPLICATION)
set(CK_CONFIG_HEADER_FILENAME application_config.h)
set(CK_BUILDINFO_HEADER_FILENAME application_buildinfo.h)

if(NOT APPLICATION_INSTALL)
    set(CK_ENABLE_INSTALL off)
endif()

if(APPLICATION_ENABLE_DEVEL)
    set(CK_ENABLE_DEVEL on)
endif()

# ----------------------------------
# Initializations
# ----------------------------------
ck_init_buildsystem()

# ----------------------------------
# Compile Definitions
# ----------------------------------
qm_add_definition(APPLICATION_NAME ${PROJECT_NAME} STRING_LITERAL)
qm_add_definition(APPLICATION_VERSION ${PROJECT_VERSION} STRING_LITERAL)
qm_add_definition(APPLICATION_ORG_NAME "OpenVPI" STRING_LITERAL)
qm_add_definition(APPLICATION_ORG_DOMAIN "org.openvpi" STRING_LITERAL)
qm_add_definition(APPLICATION_PLUGIN_IID "org.OpenVPI.${PROJECT_NAME}.Plugin" STRING_LITERAL)
qm_add_definition(APPLICATION_ENABLE_BREAKPAD CONDITION APPLICATION_ENABLE_BREAKPAD)

# ----------------------------------
# Main Project
# ----------------------------------
add_subdirectory(app)

add_subdirectory(libs)

add_subdirectory(plugins)

add_subdirectory(share)

if(APPLICATION_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# ----------------------------------
# Finale
# ----------------------------------
ck_finish_buildsystem()

# Deploy
if(APPLICATION_INSTALL)
    include("share/install.cmake")
endif()