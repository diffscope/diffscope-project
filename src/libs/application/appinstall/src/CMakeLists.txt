# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

project(appinstall VERSION 0.0.1.0)

qm_import(QtQml)
find_package(LibArchive REQUIRED)

file(GLOB_RECURSE _src CONFIGURE_DEPENDS *.h *.cpp)
ck_add_library(${PROJECT_NAME} SHARED AUTOGEN)

qm_configure_target(${PROJECT_NAME}
    SOURCES ${_src}
    LINKS ChorusKit::AppCore svscraft::Core svscraft::Quick svscraft::Network
    LINKS_PRIVATE LibArchive::LibArchive
    QT_LINKS Core Gui Network Qml Quick QuickControls2
    INCLUDE_PRIVATE *
    PREFIX APPINSTALL
)

if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

file(GLOB_RECURSE _qml_files CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.qml)
qt_add_qml_module(${PROJECT_NAME}
    URI DiffScope.AppInstall
    QML_FILES ${_qml_files}
    NAMESPACE AppInstall
    OUTPUT_DIRECTORY ${QMSETUP_BUILD_DIR}/qml/DiffScope/AppInstall
)
qm_install_qml_modules(${PROJECT_NAME})
ck_sync_include(${PROJECT_NAME} FORCE)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE
)
