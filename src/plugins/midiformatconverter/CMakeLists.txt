project(midiformatconverter
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

set(_links_icu)
if(NOT WIN32)
    find_package(ICU COMPONENTS uc REQUIRED)
    list(APPEND _links_icu ICU::uc)
else()
    list(APPEND _links_icu icu)
endif()

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
        opendspx::model
        opendspx::converter
        ${_links_icu}
    INCLUDE_PRIVATE
        internal/**
)