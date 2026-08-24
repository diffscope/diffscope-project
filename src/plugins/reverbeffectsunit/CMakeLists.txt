# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

project(reverbeffectsunit
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

find_path(SIGNALSMITH_DSP_INCLUDE_DIR
    NAMES signalsmith-dsp/delay.h
    REQUIRED
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.reverbeffectsunit
    DISPLAY_NAME "Reverb Effects Unit"
    MACRO_PREFIX REVERB_EFFECTS_UNIT
    QML_MODULE_NAME ReverbEffectsUnit
    NO_ACTION
    QT_LINKS
        Core
        Qml
        Quick
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        QAKCore
        effectsunitmanager
        talcs::Core
        svscraft::Core
        svscraft::Quick
    INCLUDE_PRIVATE
        internal/**
)

target_include_directories(${PROJECT_NAME} PRIVATE ${SIGNALSMITH_DSP_INCLUDE_DIR})
