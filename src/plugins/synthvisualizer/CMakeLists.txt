# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

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
        svscraft::Core
        svscraft::Quick
        talcs::Format
        dspxmodelORM
        uishell
        coreplugin
        audio
        synth
        visualeditor
        ScopicFlowCore
        ScopicFlowViews
    INCLUDE_PRIVATE
        internal/**
)
