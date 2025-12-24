project(libresvipformatconverter
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.libresvipformatconverter
    DISPLAY_NAME "LibreSVIP Format Converter"
    MACRO_PREFIX LIBRESVIP_FORMAT_CONVERTER
    QML_MODULE_NAME LibreSVIPFormatConverter
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
