# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

project(compressoreffectsunit
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.compressoreffectsunit
    DISPLAY_NAME "Compressor Effects Unit"
    MACRO_PREFIX COMPRESSOR_EFFECTS_UNIT
    QML_MODULE_NAME CompressorEffectsUnit
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
        ScopicFlowCore
    INCLUDE_PRIVATE
        internal/**
)
