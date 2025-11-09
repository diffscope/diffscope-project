qm_import(Translate)
qm_import(Qml)
qm_find_qt(LinguistTools)

set(CURRENT_PLUGIN_DESC "${APPLICATION_NAME} Application Plugin")

find_package(ExtensionSystem REQUIRED)

function(diffscope_add_builtin_plugin _target)
    set(options NO_TRANSLATION NO_QML_MODULE NO_ACTION)
    set(oneValueArgs NAME DISPLAY_NAME MACRO_PREFIX QML_MODULE_NAME)
    set(multiValueArgs)
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

    if(NOT FUNC_NO_ACTION)
        qak_add_action_extension(
            _actions_src
            ${CMAKE_CURRENT_SOURCE_DIR}/res/${_plugin_name}_actions.xml
            IDENTIFIER ${_target}
        )
        list(APPEND _src ${_actions_src})
    endif()

    qm_configure_target(${_target}
        SOURCES ${_src}
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
        qm_add_translation(${_target}_translations
            PREFIX
                ${_plugin_name}
            TARGETS
                ${CK_APPLICATION_TARGET} ${_target}
            LOCALES
                en_US zh_CN zh_TW ja_JP
            TS_DIR
                ${CMAKE_CURRENT_SOURCE_DIR}/res/translations
            QM_DIR
                ${CMAKE_CURRENT_BINARY_DIR}/res/translations
            TS_DEPENDS ChorusKit_UpdateTranslations
            QM_DEPENDS ChorusKit_ReleaseTranslations
        )
        ck_add_attached_files(${_target}
            SRC ${CMAKE_CURRENT_BINARY_DIR}/res/translations DEST .
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
add_subdirectory(welcomewizard)
add_subdirectory(achievement)
add_subdirectory(maintenance)