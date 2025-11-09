project(coreplugin
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME Core
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
