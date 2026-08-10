qm_import(Translate)
qm_import(Qml)
qm_find_qt(LinguistTools)

set(CURRENT_PLUGIN_DESC "${APPLICATION_NAME} Application Plugin")

find_package(ExtensionSystem CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(opendspx CONFIG REQUIRED)

function(diffscope_add_builtin_plugin _target)
    set(options NO_TRANSLATION NO_QML_MODULE NO_ACTION MACOS_INCLUDE_OBJC_SRC)
    set(oneValueArgs NAME DISPLAY_NAME MACRO_PREFIX QML_MODULE_NAME)
    set(multiValueArgs QAK_DEFINES WIN_EXCLUDE_SRC MACOS_EXCLUDE_SRC LINUX_EXCLUDE_SRC)
    cmake_parse_arguments(FUNC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(_plugin_name ${FUNC_NAME})
    set(_plugin_display_name ${FUNC_DISPLAY_NAME})

    ck_add_plugin(${_target}
        NAME ${_plugin_name}
        DISPLAY_NAME ${_plugin_display_name}
        COMPAT_VERSION 0.0.0
        VENDOR ${APPLICATION_VENDOR_NAME}
        DESCRIPTION "${PROJECT_DESCRIPTION}"
        MACRO_PREFIX ${FUNC_MACRO_PREFIX}
    )

    file(GLOB_RECURSE _src *.h *.cpp)

    if(APPLE AND FUNC_MACOS_INCLUDE_OBJC_SRC)
        file(GLOB_RECURSE _objc_src *.m *.mm)
        list(APPEND _src ${_objc_src})
    endif()

    if(WIN32 AND FUNC_WIN_EXCLUDE_SRC)
        foreach(_pattern IN LISTS FUNC_WIN_EXCLUDE_SRC)
            file(GLOB_RECURSE _win_excluded ${_pattern})
            list(REMOVE_ITEM _src ${_win_excluded})
        endforeach()
    endif()

    if(APPLE AND FUNC_MACOS_EXCLUDE_SRC)
        foreach(_pattern IN LISTS FUNC_MACOS_EXCLUDE_SRC)
            file(GLOB_RECURSE _mac_excluded ${_pattern})
            list(REMOVE_ITEM _src ${_mac_excluded})
        endforeach()
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND FUNC_LINUX_EXCLUDE_SRC)
        foreach(_pattern IN LISTS FUNC_LINUX_EXCLUDE_SRC)
            file(GLOB_RECURSE _linux_excluded ${_pattern})
            list(REMOVE_ITEM _src ${_linux_excluded})
        endforeach()
    endif()

    if(NOT FUNC_NO_ACTION)
        qak_add_action_extension(
            _actions_src
            ${CMAKE_CURRENT_SOURCE_DIR}/res/${_plugin_name}_actions.xml
            IDENTIFIER ${_target}
            DEFINES ${FUNC_QAK_DEFINES}
        )
        list(APPEND _src ${_actions_src})
    endif()

    qm_configure_target(${_target}
        SOURCES ${_src}
        INCLUDE_PRIVATE *
        ${FUNC_UNPARSED_ARGUMENTS}
    )

    if(NOT FUNC_NO_QML_MODULE)
        if(QT_KNOWN_POLICY_QTP0001)
            qt_policy(SET QTP0001 NEW)
        endif()
        if(QT_KNOWN_POLICY_QTP0004)
            qt_policy(SET QTP0004 NEW)
        endif()

        file(GLOB_RECURSE _qml_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.qml *.js *.mjs)

        qt_add_qml_module(${_target}
            URI DiffScope.${FUNC_QML_MODULE_NAME}
            QML_FILES ${_qml_files}
            OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/DiffScope/${FUNC_QML_MODULE_NAME}
        )
        qm_install_qml_modules(${_target})
    endif()

    if(NOT FUNC_NO_TRANSLATION)
        set(_translation_locales en_US zh_CN zh_TW ja_JP)
        set(_translation_qm_dir ${CMAKE_CURRENT_BINARY_DIR}/res/translations)

        qm_add_translation(${_target}_translations
            PREFIX
                ${_plugin_name}
            TARGETS
                ${CK_APPLICATION_TARGET} ${_target}
            LOCALES
                ${_translation_locales}
            TS_DIR
                ${CMAKE_CURRENT_SOURCE_DIR}/res/translations
            QM_DIR
                ${_translation_qm_dir}
            TS_DEPENDS ChorusKit_UpdateTranslations
            QM_DEPENDS ChorusKit_ReleaseTranslations
        )

        set(_translation_output_dir $<TARGET_FILE_DIR:${_target}>/translations)
        set(_translation_deploy_stamp_dir
            ${CMAKE_CURRENT_BINARY_DIR}/deploy-translations/$<CONFIG>
        )
        set(_translation_deploy_stamps)
        foreach(_locale IN LISTS _translation_locales)
            set(_translation_qm_file ${_translation_qm_dir}/${_plugin_name}_${_locale}.qm)
            set(_deployed_translation_qm_file
                ${_translation_output_dir}/${_plugin_name}_${_locale}.qm
            )
            set(_translation_deploy_stamp
                ${_translation_deploy_stamp_dir}/${_plugin_name}_${_locale}.stamp
            )

            add_custom_command(
                OUTPUT ${_translation_deploy_stamp}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${_translation_output_dir}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${_translation_deploy_stamp_dir}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${_translation_qm_file}
                    ${_deployed_translation_qm_file}
                COMMAND ${CMAKE_COMMAND} -E touch ${_translation_deploy_stamp}
                DEPENDS ${_translation_qm_file}
                COMMENT "Deploying ${_plugin_name}_${_locale}.qm"
                VERBATIM
            )
            list(APPEND _translation_deploy_stamps ${_translation_deploy_stamp})
        endforeach()

        add_custom_target(${_target}_deploy_translations ALL
            DEPENDS ${_translation_deploy_stamps}
        )
        add_dependencies(${CK_APPLICATION_TARGET} ${_target}_deploy_translations)

        ck_add_attached_files(${_target}
            SKIP_BUILD
            SRC ${_translation_qm_dir} DEST .
        )
    endif()

    ck_sync_include(${_target}
        OPTIONS
        EXCLUDE "${_target}/internal/.+"
        FORCE
    )

    ck_sync_include(${_target}
        DIRECTORY
        internal
        PREFIX "${_target}/internal"
        SKIP_INSTALL
        FORCE
    )
endfunction()

add_subdirectory(coreplugin)
add_subdirectory(audio)
add_subdirectory(synth)
add_subdirectory(welcomewizard)
add_subdirectory(maintenance)
add_subdirectory(packagemanager)
add_subdirectory(importexportmanager)
add_subdirectory(visualeditor)
add_subdirectory(audiovisualizer)
add_subdirectory(midiformatconverter)
add_subdirectory(libresvipformatconverter)
