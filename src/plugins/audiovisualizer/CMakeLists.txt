project(audiovisualizer
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.audiovisualizer
    DISPLAY_NAME "Audio Visualizer"
    MACRO_PREFIX AUDIO_VISUALIZER
    QML_MODULE_NAME AudioVisualizer
    QT_LINKS
        Core
        Qml
        Quick
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        QAKCore
        coreplugin
        audio
        talcs::Format
        svscraft::Core
        svscraft::Quick
        visualeditor
    INCLUDE_PRIVATE
        internal/**
)
