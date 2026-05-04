project(tst_uishell_PackageManagerView)

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

file(GLOB _src *.h *.cpp *.qrc)
file(GLOB_RECURSE _qml_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.qml *.js *.mjs)

add_executable(${PROJECT_NAME} ${_src})

qm_configure_target(${PROJECT_NAME}
    LINKS svscraft::Gui svscraft::Quick svscraft::FluentSystemIcons uishell
    QT_LINKS Core Gui Qml Quick QuickControls2
)

if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()

qt_add_qml_module(${PROJECT_NAME}
    URI DiffScope.UIShell.Test.PackageManagerView
    QML_FILES ${_qml_files}
    RESOURCES
        avatar_alpha.png
        background_alpha.png
        avatar_alpha_echo.png
        background_alpha_echo.png
        avatar_beta.png
        background_beta.png
        avatar_beta_harmonic.png
        background_beta_harmonic.png
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE
)
