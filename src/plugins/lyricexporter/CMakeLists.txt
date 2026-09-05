# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

project(lyricexporter
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.lyricexporter
    DISPLAY_NAME "Lyric Exporter"
    MACRO_PREFIX LYRIC_EXPORTER
    QML_MODULE_NAME LyricExporter
    NO_ACTION
    QT_LINKS
        Core
        Gui
        Qml
        Quick
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        coreplugin
        importexportmanager
        opendspx::model
        svscraft::Core
        svscraft::Quick
    INCLUDE_PRIVATE
        internal/**
)
