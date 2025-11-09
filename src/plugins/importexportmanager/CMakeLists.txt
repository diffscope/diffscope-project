project(importexportmanager
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.importexportmanager
    DISPLAY_NAME "Import/Export Manager"
    MACRO_PREFIX IMPORT_EXPORT_MANAGER
    QML_MODULE_NAME ImportExportManager
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
        QAKCore
        QAKQuick
        coreplugin
        opendspx::model
    INCLUDE_PRIVATE
        internal/**
)