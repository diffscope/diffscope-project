# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

project(deessereffectsunit
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

find_path(SIGNALSMITH_DSP_INCLUDE_DIR
    NAMES signalsmith-dsp/filters.h
    REQUIRED
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.deessereffectsunit
    DISPLAY_NAME "De-Esser Effects Unit"
    MACRO_PREFIX DEESSER_EFFECTS_UNIT
    QML_MODULE_NAME DeEsserEffectsUnit
    NO_ACTION
    NO_TRANSLATION
    QT_LINKS
        Core
        Gui
        Qml
        Quick
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        QAKCore
        audio
        talcs::Core
        svscraft::Core
        svscraft::Quick
        ScopicFlowCore
    INCLUDE_PRIVATE
        internal/**
)

target_include_directories(${PROJECT_NAME} PRIVATE ${SIGNALSMITH_DSP_INCLUDE_DIR})
