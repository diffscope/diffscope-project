# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

project(channelmappereffectsunit
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.channelmappereffectsunit
    DISPLAY_NAME "Channel Mapper Effects Unit"
    MACRO_PREFIX CHANNEL_MAPPER_EFFECTS_UNIT
    QML_MODULE_NAME ChannelMapperEffectsUnit
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
