# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

project(midiformatconverter
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

find_package(ICU COMPONENTS uc i18n REQUIRED)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.midiformatconverter
    DISPLAY_NAME "MIDI Format Converter"
    MACRO_PREFIX MIDI_FORMAT_CONVERTER
    QML_MODULE_NAME MIDIFormatConverter
    NO_ACTION
    QT_LINKS
        Core
        Gui
        Widgets
        Qml
        Quick
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        uishell
        coreplugin
        importexportmanager
        SVSCraftCore
        opendspx::model
        opendspx::converter
        ICU::uc
        ICU::i18n
    INCLUDE_PRIVATE
        internal/**
)
