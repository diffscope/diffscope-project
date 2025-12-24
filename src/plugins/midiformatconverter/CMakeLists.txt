project(midiformatconverter
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

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
    INCLUDE_PRIVATE
        internal/**
)