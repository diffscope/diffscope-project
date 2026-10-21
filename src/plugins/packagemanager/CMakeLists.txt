# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

project(packagemanager
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

set(DIFFSCOPE_DSPM_CATALOG_URL "https://catalogs.diffscope.org/v1/dspm/" CACHE STRING
    "URL of the DiffScope Package Manager release catalog")

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
        appinstall
        uishell
        QAKCore
        QAKQuick
        coreplugin
    INCLUDE_PRIVATE
        internal/**
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    DIFFSCOPE_DSPM_CATALOG_URL="${DIFFSCOPE_DSPM_CATALOG_URL}"
)

if(DEFINED DIFFSCOPE_PACKAGE_MANAGER_COMMAND_LINE_TOOL)
    ck_add_attached_files(${PROJECT_NAME}
        SRC ${DIFFSCOPE_PACKAGE_MANAGER_COMMAND_LINE_TOOL} DEST ./dspm
    )
endif()
