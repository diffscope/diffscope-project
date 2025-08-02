project(coreplugin
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

set(_plugin_name Core)
set(_plugin_display_name Core)

# Add target
ck_add_plugin(${PROJECT_NAME}
    NAME ${_plugin_name}
    DISPLAY_NAME ${_plugin_display_name}
    COMPAT_VERSION 0.0.0.0
    VENDOR "Team OpenVPI"
    DESCRIPTION "${PROJECT_DESCRIPTION}"
    MACRO_PREFIX CORE
)

find_package(ExtensionSystem REQUIRED)

# Configure target
file(GLOB_RECURSE _src *.h *.cpp)
qak_add_action_extension(
    _core_actions_src
    res/core_actions.xml
)
list(APPEND _src ${_core_actions_src})
qm_configure_target(${PROJECT_NAME}
    SOURCES ${_src}
    QT_LINKS
        Core
        Gui
        Widgets
        Qml
        Quick
        QuickControls2
        QuickTemplates2
    QT_INCLUDE_PRIVATE
        Core
        Gui
        Widgets
        Qml
        Quick
        QuickControls2
        QuickTemplates2
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        loadapi
        uishell
        uishellplugin
        QAKCore
        QAKQuick
        # ChorusKit::AppCore
        # JetBrainsDockingSystem
        # dspxmodel
        # opendspx::opendspx
    INCLUDE_PRIVATE
        core
        windows
        internal/**
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

qt_add_qml_module(${PROJECT_NAME}
    URI DiffScope.CorePlugin
    QML_FILES ${_qml_files}
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/DiffScope/CorePlugin
)

# Add translation
# qm_add_translation(${PROJECT_NAME}_translations
#     PREFIX
#         ${_plugin_name}
#     TARGETS
#         ${CK_APPLICATION_NAME} ${PROJECT_NAME}
#     LOCALES
#         zh_CN zh_TW ja_JP
#     TS_DIR
#         res/translations
#     QM_DIR
#         ${CMAKE_CURRENT_BINARY_DIR}/res/translations
#     TS_DEPENDS ChorusKit_UpdateTranslations
#     QM_DEPENDS ChorusKit_ReleaseTranslations
# )

# Add resources
# ck_add_attached_files(${PROJECT_NAME}
#     SRC res/icons DEST .
#     SRC res/themes DEST .
#     SRC ${CMAKE_CURRENT_BINARY_DIR}/res/translations DEST .
# )

file(GLOB _icons res/icons/*)

qt_add_resources(${PROJECT_NAME} icons
    PREFIX /diffscope/coreplugin/icons
    BASE res/icons
    FILES ${_icons}
)

# Add install command
ck_sync_include(${PROJECT_NAME}
    OPTIONS
        EXCLUDE "${PROJECT_NAME}/internal/.+"
        FORCE
)

ck_sync_include(${PROJECT_NAME}
    DIRECTORY
        internal
    PREFIX "${PROJECT_NAME}/internal"
    SKIP_INSTALL
)