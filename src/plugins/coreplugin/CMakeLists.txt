project(coreplugin
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

set(_qak_not_macos)
if(APPLE)
    set(_qak_not_macos false)
else()
    set(_qak_not_macos true)
endif()

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.core
    DISPLAY_NAME Core
    MACRO_PREFIX CORE
    QML_MODULE_NAME Core
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
        dspxmodel
        opendspx::model
        opendspx::serializer
    INCLUDE_PRIVATE
        core
        windows
        internal/**
    QAK_DEFINES
        NOT_MACOS=${_qak_not_macos}
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
