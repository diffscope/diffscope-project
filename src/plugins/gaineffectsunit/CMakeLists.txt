# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

project(gaineffectsunit
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.gaineffectsunit
    DISPLAY_NAME "Gain Effects Unit"
    MACRO_PREFIX GAIN_EFFECTS_UNIT
    QML_MODULE_NAME GainEffectsUnit
    NO_ACTION
    QT_LINKS
        Core
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
    INCLUDE_PRIVATE
        internal/**
)
