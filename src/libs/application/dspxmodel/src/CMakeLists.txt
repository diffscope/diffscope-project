project(dspxmodel VERSION ${APPLICATION_VERSION})

qm_import(Qml)

file(GLOB_RECURSE _src *.h *.cpp)

ck_add_library(${PROJECT_NAME} SHARED AUTOGEN)

qm_configure_target(${PROJECT_NAME}
    SOURCES ${_src}
    LINKS
    QT_LINKS Core Gui Qml
    QT_INCLUDE_PRIVATE Core Gui Qml
    INCLUDE_PRIVATE *
    PREFIX DSPX_MODEL
)

if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

file(GLOB_RECURSE _qml_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.qml *.js *.mjs)

qt_add_qml_module(${PROJECT_NAME}
    URI DiffScope.DspxModel
    QML_FILES ${_qml_files}
    NAMESPACE dspx
    OUTPUT_DIRECTORY ${QMSETUP_BUILD_DIR}/qml/DiffScope/DspxModel
)

qm_install_qml_modules(${PROJECT_NAME})

ck_sync_include(${PROJECT_NAME} FORCE)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE
)

