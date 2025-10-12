project(maintenance
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

set(_plugin_name Maintenance)
set(_plugin_display_name "Maintenance")

# Add target
ck_add_plugin(${PROJECT_NAME}
    NAME ${_plugin_name}
    DISPLAY_NAME ${_plugin_display_name}
    COMPAT_VERSION 0.2.0
    VENDOR ${APPLICATION_VENDOR_NAME}
    DESCRIPTION "${PROJECT_DESCRIPTION}"
    MACRO_PREFIX MAINTENANCE
)

find_package(ExtensionSystem REQUIRED)

# Configure target
file(GLOB_RECURSE _src *.h *.cpp)
qak_add_action_extension(
    _maintenance_actions_src
    res/maintenance_actions.xml
)
list(APPEND _src ${_maintenance_actions_src})

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
        Network
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
        QAKCore
        QAKQuick
        svscraft::QmlExtras
        svscraft::Core
        coreplugin
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

file(GLOB_RECURSE _qml_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.qml *.js *.mjs)

qt_add_qml_module(${PROJECT_NAME}
    URI DiffScope.Maintenance
    QML_FILES ${_qml_files}
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/DiffScope/Maintenance
)

# Add translation
qm_add_translation(${PROJECT_NAME}_translations
    PREFIX
        ${_plugin_name}
    TARGETS
        ${CK_APPLICATION_TARGET} ${PROJECT_NAME}
    LOCALES
        en_US zh_CN zh_TW ja_JP
    TS_DIR
        res/translations
    QM_DIR
        ${CMAKE_CURRENT_BINARY_DIR}/res/translations
    TS_DEPENDS ChorusKit_UpdateTranslations
    QM_DEPENDS ChorusKit_ReleaseTranslations
)

# Add resources
ck_add_attached_files(${PROJECT_NAME}
    SRC ${CMAKE_CURRENT_BINARY_DIR}/res/translations DEST .
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