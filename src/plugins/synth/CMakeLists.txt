project(synth
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.synth
    DISPLAY_NAME Synthesis
    MACRO_PREFIX SYNTH
    QML_MODULE_NAME Synth
    QT_LINKS
        Core
        Concurrent
        Gui
        Network
        Qml
        Quick
        QuickDialogs2
        Widgets
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        QAKCore
        QAKQuick
        uishell
        coreplugin
        audio
        svscraft::Core
        svscraft::Quick
        svscraft::QmlExtras
        SVSCraft3rdParty_tinyexpr
    INCLUDE_PRIVATE
        internal/**
)
