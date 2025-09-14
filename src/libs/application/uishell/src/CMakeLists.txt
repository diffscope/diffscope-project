project(uishell VERSION 0.0.1.0)

qm_import(Qml)

file(GLOB_RECURSE _src *.h *.cpp)

find_package(QWindowKit REQUIRED)
find_package(ExtensionSystem REQUIRED)

ck_add_library(${PROJECT_NAME} SHARED AUTOGEN)

qm_configure_target(${PROJECT_NAME}
    SOURCES ${_src}
    LINKS
        svscraft::Gui svscraft::Quick
        QWindowKit::Core QWindowKit::Quick
        ChorusKit::AppCore
        ExtensionSystem::ExtensionSystem
        QActionKit::Core QActionKit::Quick
    QT_LINKS Core Gui Widgets Qml Quick QuickControls2
    QT_INCLUDE_PRIVATE Core Gui Widgets Qml Quick QuickControls2
    INCLUDE_PRIVATE *
    PREFIX UISHELL
)

if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

file(GLOB_RECURSE _qml_files_abs *.qml *.js *.mjs)
set(_qml_files)
foreach(_file IN LISTS _qml_files_abs)
    file(RELATIVE_PATH _rel_path ${CMAKE_CURRENT_SOURCE_DIR} ${_file})
    list(APPEND _qml_files ${_rel_path})
endforeach()
file(GLOB_RECURSE _resource_files_abs assets/*)
set(_resource_files)
foreach(_file IN LISTS _resource_files_abs)
    file(RELATIVE_PATH _rel_path ${CMAKE_CURRENT_SOURCE_DIR} ${_file})
    list(APPEND _resource_files ${_rel_path})
endforeach()

qt_add_qml_module(${PROJECT_NAME}
    URI DiffScope.UIShell
    QML_FILES ${_qml_files}
    RESOURCES ${_resource_files}
    NAMESPACE UIShell
    OUTPUT_DIRECTORY ${QMSETUP_BUILD_DIR}/qml/DiffScope/UIShell
)

qm_install_qml_modules(${PROJECT_NAME})

ck_sync_include(${PROJECT_NAME} FORCE)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE
)

