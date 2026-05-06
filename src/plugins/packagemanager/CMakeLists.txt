project(packagemanager
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.packagemanager
    DISPLAY_NAME "Package Manager"
    MACRO_PREFIX PACKAGE_MANAGER
    QML_MODULE_NAME PackageManager
    QT_LINKS
        Core
        Gui
        Widgets
        Qml
        Quick
        QuickDialogs2
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        uishell
        QAKCore
        QAKQuick
        coreplugin
    INCLUDE_PRIVATE
        internal/**
)

if(DEFINED DIFFSCOPE_PACKAGE_MANAGER_COMMAND_LINE_TOOL)
    ck_add_attached_files(${PROJECT_NAME}
        SRC ${DIFFSCOPE_PACKAGE_MANAGER_COMMAND_LINE_TOOL} DEST ./dspm
    )
endif()
