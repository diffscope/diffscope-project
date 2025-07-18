project(tst_uishell_ProjectWindow)

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

file(GLOB _src *.h *.cpp *.qrc)
file(GLOB_RECURSE _qml_files_abs *.qml *.js *.mjs)
set(_qml_files)
foreach(_file IN LISTS _qml_files_abs)
    file(RELATIVE_PATH _rel_path ${CMAKE_CURRENT_SOURCE_DIR} ${_file})
    list(APPEND _qml_files ${_rel_path})
endforeach()

add_executable(${PROJECT_NAME} ${_src})

qm_configure_target(${PROJECT_NAME}
    LINKS svscraft::Gui svscraft::Quick SVSCraftQuickplugin uishell uishellplugin
    QT_LINKS Core Gui Qml Quick QuickControls2
)

if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()

qt_add_qml_module(${PROJECT_NAME}
    URI DiffScope.UIShell.Test.ProjectWindow
    QML_FILES ${_qml_files}
    RESOURCES test_icon.png
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE
)