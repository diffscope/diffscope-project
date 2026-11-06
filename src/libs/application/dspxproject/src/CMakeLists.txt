# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

project(dspxproject VERSION ${APPLICATION_VERSION})

qm_import(QtQml)
find_package(opendspx CONFIG REQUIRED)

file(GLOB_RECURSE _src CONFIGURE_DEPENDS *.h *.cpp)
ck_add_library(${PROJECT_NAME} SHARED AUTOGEN)

qm_configure_target(${PROJECT_NAME}
    SOURCES ${_src}
    LINKS
        ChorusKit::AppCore
        svscraft::Core
        dspxmodelCore
        dspxmodelORM
        dspxmodelSelectionModel
        opendspx::model
        singercatalog
        transactional
    LINKS_PRIVATE
        svscraft::Qml
        opendspx::serializer
        jsonconv::stdcorelib
    QT_LINKS Core Gui Qml Quick
    INCLUDE_PRIVATE *
    PREFIX DSPXPROJECT
)

if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()

qt_add_qml_module(${PROJECT_NAME}
    URI DiffScope.DspxProject
    NAMESPACE DspxProject
    OUTPUT_DIRECTORY ${QMSETUP_BUILD_DIR}/qml/DiffScope/DspxProject
)

qm_install_qml_modules(${PROJECT_NAME})
ck_sync_include(${PROJECT_NAME} FORCE)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE
)
