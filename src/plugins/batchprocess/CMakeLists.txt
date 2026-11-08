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
        dspxproject
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
    LINKS_PRIVATE
        jsonconv::stdcorelib
    INCLUDE_PRIVATE
        internal/**
    QML_EXCLUDE_SRC
        res/builtin/*
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

set(_builtin_output_dir $<TARGET_FILE_DIR:${PROJECT_NAME}>/builtin)
set(_builtin_deploy_stamp_dir ${CMAKE_CURRENT_BINARY_DIR}/deploy-builtin/$<CONFIG>)
set(_builtin_deploy_stamps)
foreach(_script IN LISTS _builtin_scripts)
    file(RELATIVE_PATH _script_relative_path "${CMAKE_CURRENT_SOURCE_DIR}/res/builtin" "${_script}")
    get_filename_component(_script_relative_dir "${_script_relative_path}" DIRECTORY)
    set(_script_deploy_stamp "${_builtin_deploy_stamp_dir}/${_script_relative_path}.stamp")

    add_custom_command(
        OUTPUT "${_script_deploy_stamp}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_builtin_output_dir}/${_script_relative_dir}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_builtin_deploy_stamp_dir}/${_script_relative_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_script}"
            "${_builtin_output_dir}/${_script_relative_path}"
        COMMAND ${CMAKE_COMMAND} -E touch "${_script_deploy_stamp}"
        DEPENDS "${_script}"
        COMMENT "Deploying built-in script file ${_script_relative_path}"
        VERBATIM
    )
    list(APPEND _builtin_deploy_stamps "${_script_deploy_stamp}")
endforeach()

add_custom_target(${PROJECT_NAME}_deploy_builtin ALL
    DEPENDS ${_builtin_deploy_stamps}
)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_deploy_builtin)
add_dependencies(${CK_APPLICATION_TARGET} ${PROJECT_NAME}_deploy_builtin)

ck_add_attached_files(${PROJECT_NAME}
    SKIP_BUILD
    SRC "${CMAKE_CURRENT_SOURCE_DIR}/res/builtin" DEST .
)
