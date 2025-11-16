project(audio
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.audio
    DISPLAY_NAME Audio
    MACRO_PREFIX AUDIO
    QML_MODULE_NAME Audio
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
        talcs::Core
        talcs::Device
        talcs::Format
    INCLUDE_PRIVATE
        internal/**
)
