project(coreplugin
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

qm_import(Qml)

set(_plugin_name Core)
set(_plugin_display_name Core)

# Add target
ck_add_plugin(${PROJECT_NAME}
    NAME ${_plugin_name}
    DISPLAY_NAME ${_plugin_display_name}
    COMPAT_VERSION 0.2.0
    VENDOR ${APPLICATION_VENDOR_NAME}
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
        QuickDialogs2
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
        # ChorusKit::AppCore
        # JetBrainsDockingSystem
        dspxmodel
        opendspx::model
        opendspx::serializer
        ScopicFlowCore
        ScopicFlowViews
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
    URI DiffScope.Core
    QML_FILES ${_qml_files}
    OUTPUT_DIRECTORY ${QMSETUP_BUILD_DIR}/qml/DiffScope/Core
)

qm_install_qml_modules(${PROJECT_NAME})

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

file(GLOB _res_icons res/icons/*)

qt_add_resources(${PROJECT_NAME} icons
    PREFIX /diffscope/coreplugin/icons
    BASE res/icons
    FILES ${_res_icons}
)

file(GLOB _res_logos res/logos/*)

qt_add_resources(${PROJECT_NAME} logos
    PREFIX /diffscope/coreplugin/logos
    BASE res/logos
    FILES ${_res_logos}
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