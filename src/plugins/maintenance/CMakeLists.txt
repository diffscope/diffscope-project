project(maintenance
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.maintenance
    DISPLAY_NAME Maintenance
    MACRO_PREFIX MAINTENANCE
    QML_MODULE_NAME Maintenance
    QT_LINKS
        Core
        Gui
        Widgets
        Qml
        Quick
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        uishell
        QAKCore
        QAKQuick
        coreplugin
    INCLUDE_PRIVATE
        internal/**
)
