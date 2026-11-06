# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

project(singercatalog VERSION ${APPLICATION_VERSION})

qm_import(QtQml)

file(GLOB_RECURSE _src CONFIGURE_DEPENDS *.h *.cpp)
ck_add_library(${PROJECT_NAME} SHARED AUTOGEN)

qm_configure_target(${PROJECT_NAME}
    SOURCES ${_src}
    LINKS dspxmodelORM
    QT_LINKS Core Qml
)

if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()

qt_add_qml_module(${PROJECT_NAME}
    URI DiffScope.SingerCatalog
    NAMESPACE SingerCatalog
    DEPENDENCIES DiffScope.DspxModel
    OUTPUT_DIRECTORY ${QMSETUP_BUILD_DIR}/qml/DiffScope/SingerCatalog
)

qm_install_qml_modules(${PROJECT_NAME})
ck_sync_include(${PROJECT_NAME} OPTIONS FORCE)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE
)
