project(achievement
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.achievement
    DISPLAY_NAME Achievement
    MACRO_PREFIX ACHIEVEMENT
    QML_MODULE_NAME Achievement
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
