cmake_minimum_required(VERSION 3.19)

project(DiffScope VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# ----------------------------------
# Build Options
# ----------------------------------
option(APPLICATION_BUILD_TESTS "Build test cases" OFF)
option(APPLICATION_INSTALL "Install this project" ON)
option(APPLICATION_ENABLE_DEVEL "Install developer files" OFF)
option(APPLICATION_INSTALL_MSVC_RUNTIME "Install MSVC runtime" OFF)
option(APPLICATION_CONFIGURE_INSTALLER "Configure installer" OFF)

# ----------------------------------
# Application Options
# ----------------------------------
option(APPLICATION_ENABLE_BREAKPAD "Enable google breakpad" ON)

# ----------------------------------
# CMake Settings
# ----------------------------------
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
else()
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
endif()

if(APPLICATION_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
endif()

# ----------------------------------
# Project Variables
# ----------------------------------
set(APPLICATION_TARGET ${PROJECT_NAME})
if(NOT DEFINED APPLICATION_NAME)
    set(APPLICATION_NAME ${PROJECT_NAME})
endif()
if(NOT DEFINED APPLICATION_DISPLAY_NAME)
    set(APPLICATION_DISPLAY_NAME ${PROJECT_NAME})
endif()
if(NOT DEFINED APPLICATION_SEMVER)
    set(APPLICATION_SEMVER ${PROJECT_VERSION})
endif()
set(APPLICATION_VERSION ${PROJECT_VERSION})
set(APPLICATION_INSTALL_NAME ${APPLICATION_NAME})
set(APPLICATION_VENDOR_NAME "Team OpenVPI")
set(APPLICATION_DEV_START_YEAR 2019)
set(APPLICATION_URL "https://diffscope.org/")
set(APPLICATION_SUPPORT_URL "https://help.diffscope.org/diffscope/${PROJECT_VERSION}/")

# ----------------------------------
# Find basic dependencies
# ----------------------------------
find_package(qmsetup CONFIG REQUIRED)

qm_import(Filesystem)

if(NOT DEFINED QMSETUP_BUILD_DIR)
    set(QMSETUP_BUILD_DIR "${CMAKE_BINARY_DIR}/out")
endif()

qm_init_directories()

# ----------------------------------
# Add source modules
# ----------------------------------
add_subdirectory(src)

add_subdirectory(dist)