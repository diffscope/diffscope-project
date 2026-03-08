project(directmanipulation
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.directmanipulation
    DISPLAY_NAME "Win32 Direct Manipulation Helper"
    MACRO_PREFIX DIRECT_MANIPULATION
    NO_QML_MODULE
    NO_ACTION
    NO_TRANSLATION
    QT_LINKS
        Core
        Gui
        Widgets
        Qml
        Quick
    QT_INCLUDE_PRIVATE
        Core
        Gui
        Qml
        Quick
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        QtWin32DirectManipulateHelper::Core
        loadapi
        coreplugin
    INCLUDE_PRIVATE
        internal/**
)
