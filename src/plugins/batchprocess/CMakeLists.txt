# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

project(batchprocess
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.batchprocess
    DISPLAY_NAME "Batch Process"
    MACRO_PREFIX BATCH_PROCESS
    QML_MODULE_NAME BatchProcess
    QT_LINKS
        Core
        Gui
        Widgets
        Qml
        Quick
        QuickDialogs2
    QT_INCLUDE_PRIVATE
        Qml
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        uishell
        QAKCore
        QAKQuick
        coreplugin
        svscraft::Gui
        svscraft::Quick
        svscraft::FluentSystemIcons
    INCLUDE_PRIVATE
        internal/**
)

file(GLOB_RECURSE _internal_scripts "${CMAKE_CURRENT_SOURCE_DIR}/res/internalscripts/*")

qt_add_resources(${PROJECT_NAME} batchprocess_runtime
    PREFIX "/diffscope/batchprocess/internalscripts"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/res/internalscripts"
    FILES ${_internal_scripts}
)
