# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

project(equalizereffectsunit
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

find_path(SIGNALSMITH_DSP_INCLUDE_DIR
    NAMES signalsmith-dsp/filters.h
    REQUIRED
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.equalizereffectsunit
    DISPLAY_NAME "Equalizer Effects Unit"
    MACRO_PREFIX EQUALIZER_EFFECTS_UNIT
    QML_MODULE_NAME EqualizerEffectsUnit
    NO_ACTION
    QT_LINKS
        Core
        Gui
        Qml
        Quick
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        QAKCore
        coreplugin
        effectsunitmanager
        talcs::Core
        svscraft::Core
        svscraft::Quick
    INCLUDE_PRIVATE
        internal/**
)

target_include_directories(${PROJECT_NAME} PRIVATE ${SIGNALSMITH_DSP_INCLUDE_DIR})
