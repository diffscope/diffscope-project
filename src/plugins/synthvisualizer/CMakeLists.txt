project(synthvisualizer
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.synthvisualizer
    DISPLAY_NAME "Synthesis Visualizer"
    MACRO_PREFIX SYNTH_VISUALIZER
    QML_MODULE_NAME SynthVisualizer
    QT_LINKS
        Core
        Qml
        Quick
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        QAKCore
        QAKQuick
        svscraft::Quick
        dspxmodelORM
        uishell
        coreplugin
        synth
        visualeditor
        ScopicFlowCore
        ScopicFlowViews
    INCLUDE_PRIVATE
        internal/**
)
