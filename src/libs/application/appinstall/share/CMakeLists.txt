# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

set(_locales)
foreach(_locale IN ITEMS en_US zh_CN zh_TW ja_JP)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/translations/appinstall_${_locale}.ts")
        list(APPEND _locales ${_locale})
    endif()
endforeach()
if(NOT _locales)
    return()
endif()

qm_import(QtLinguist)
qm_find_qt(LinguistTools)

set(_qm_dir ${CMAKE_BUILD_SHARE_DIR}/appinstall/translations)
qm_add_translation(appinstall_translations
    LOCALES ${_locales}
    PREFIX appinstall
    TARGETS appinstall
    TS_DIR translations
    QM_DIR ${_qm_dir}
    TS_DEPENDS ChorusKit_UpdateTranslations
    QM_DEPENDS ChorusKit_ReleaseTranslations
)

if(APPLICATION_INSTALL)
    install(DIRECTORY ${_qm_dir}/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/appinstall/translations
        FILES_MATCHING PATTERN "*.qm"
    )
endif()
