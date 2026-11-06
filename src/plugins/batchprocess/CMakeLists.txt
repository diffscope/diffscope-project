# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

project(batchprocess
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

find_package(i18next CONFIG REQUIRED)

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
        singercatalog
        dspxmodelCore
        dspxmodelORM
        dspxmodelSelectionModel
        opendspx::model
        opendspx::serializer
        QJSEnginePolyfills::Intl
        transactional
        svscraft::Gui
        svscraft::Quick
        svscraft::FluentSystemIcons
    INCLUDE_PRIVATE
        internal/**
)

file(GLOB_RECURSE _internal_scripts CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/res/internalscripts/*")
file(GLOB_RECURSE _builtin_scripts CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/res/builtin/*")

get_filename_component(_i18next_module_dir "${i18next_MODULE_FILE}" DIRECTORY)

qt_add_resources(${PROJECT_NAME} batchprocess_i18next
    PREFIX "/diffscope/batchprocess/i18next"
    BASE "${_i18next_module_dir}"
    FILES "${i18next_MODULE_FILE}"
)

qt_add_resources(${PROJECT_NAME} batchprocess_runtime
    PREFIX "/diffscope/batchprocess/internalscripts"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/res/internalscripts"
    FILES ${_internal_scripts}
)

qt_add_resources(${PROJECT_NAME} batchprocess_builtin
    PREFIX "/diffscope/batchprocess/builtin"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/res/builtin"
    FILES ${_builtin_scripts}
)
